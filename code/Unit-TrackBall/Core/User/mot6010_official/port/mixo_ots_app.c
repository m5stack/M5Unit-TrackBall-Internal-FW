#include "mixo_ots_app.h"

#include <stdbool.h>

#include "main.h"
#include "mixo_gpio_i2c.h"
#include "mixo_hal.h"
#include "mixo_hal_timer.h"
#include "mixo_ots_access.h"
#include "mixo_ots_driver.h"

#define MIXO_OTS_I2C_ADDR_7BIT       (0x74U)
#define MIXO_OTS_POLL_PERIOD_MS      (10U)
#define MIXO_OTS_IDLE_POLL_LIMIT     (20U)
#define MIXO_OTS_MOTION_FLAG         (0x80U)
#define MIXO_OTS_OVERFLOW_MASK       (0x18U)
#define MIXO_OTS_INIT_RETRY_COUNT    (3U)
#define MIXO_OTS_INIT_RETRY_DELAY_MS (10U)

volatile mixo_ots_data_t mixo_ots_data;

static uint32_t last_poll_ms;
static volatile uint8_t irq_pending;
static uint8_t polling_active;
static uint8_t sensor_initialized;
static uint16_t idle_poll_count;
static volatile uint8_t sensor_init_status;
static volatile uint8_t reinitialize_pending;
static volatile uint8_t axis_config_pending;
static volatile uint8_t axis_config_request;
static volatile uint8_t i2c_error_clear_pending;
static volatile uint8_t accum_clear_pending;
static volatile uint8_t motion_clear_pending;
static uint8_t axis_config;

static int32_t mixo_ots_accumulate_saturated(int32_t accumulated, int16_t delta)
{
    int32_t delta32 = (int32_t)delta;

    if ((delta32 > 0) && (accumulated > (INT32_MAX - delta32))) {
        return INT32_MAX;
    }
    if ((delta32 < 0) && (accumulated < (INT32_MIN - delta32))) {
        return INT32_MIN;
    }

    return accumulated + delta32;
}

static void mixo_ots_update_data(int16_t dx, int16_t dy, uint8_t ready)
{
    __disable_irq();
    mixo_ots_data.dx = dx;
    mixo_ots_data.dy = dy;
    if (ready != 0U) {
        mixo_ots_data.dx_accum = mixo_ots_accumulate_saturated(mixo_ots_data.dx_accum, dx);
        mixo_ots_data.dy_accum = mixo_ots_accumulate_saturated(mixo_ots_data.dy_accum, dy);
    }
    mixo_ots_data.ready = ready;
    if (ready != 0U) {
        mixo_ots_data.ready_generation++;
        mixo_ots_data.dx_accum_generation++;
        mixo_ots_data.dy_accum_generation++;
    }
    __enable_irq();
}

static void mixo_ots_clear_motion_data(void)
{
    __disable_irq();
    mixo_ots_data.dx       = 0;
    mixo_ots_data.dy       = 0;
    mixo_ots_data.dx_accum = 0;
    mixo_ots_data.dy_accum = 0;
    mixo_ots_data.ready    = 0U;
    mixo_ots_data.ready_generation++;
    mixo_ots_data.dx_accum_generation++;
    mixo_ots_data.dy_accum_generation++;
    __enable_irq();
}

static void mixo_ots_clear_accum(void)
{
    __disable_irq();
    mixo_ots_data.dx_accum = 0;
    mixo_ots_data.dy_accum = 0;
    mixo_ots_data.dx_accum_generation++;
    mixo_ots_data.dy_accum_generation++;
    __enable_irq();
}

static void mixo_ots_clear_motion_delta(void)
{
    __disable_irq();
    mixo_ots_data.dx    = 0;
    mixo_ots_data.dy    = 0;
    mixo_ots_data.ready = 0U;
    mixo_ots_data.ready_generation++;
    __enable_irq();
}

static void mixo_ots_power_on(void)
{
    HAL_GPIO_WritePin(EN_1V8_GPIO_Port, EN_1V8_Pin, GPIO_PIN_SET);
    HAL_Delay(10U);
    HAL_GPIO_WritePin(EN_3V3_GPIO_Port, EN_3V3_Pin, GPIO_PIN_SET);
    HAL_Delay(10U);
}

static void mixo_ots_start_polling(void)
{
    mixo_ots_mot_exti_deinit();
    polling_active  = 1U;
    idle_poll_count = 0U;
    last_poll_ms    = 0U;
}

static uint8_t mixo_ots_try_init(void)
{
    if (mixo_ots_init() != 0) {
        return 0U;
    }

    if (mixo_ots_forced_clear_int() != 0) {
        return 0U;
    }
    if (mixo_ots_apply_axis_config(axis_config) != 0) {
        return 0U;
    }
    mixo_ots_mot_exti_init();
    /* A motion edge may occur between status clearing and EXTI enable. */
    if (mixo_hal_get_mot_level()) {
        mixo_ots_mot_exti_deinit();
        irq_pending = 1U;
    }
    return 1U;
}

static void mixo_ots_pending_service(void)
{
    int config_result;
    uint8_t initialized;
    uint8_t requested_axis_config;
    uint8_t axis_request_pending;
    uint8_t reinitialize_requested;
    uint8_t error_clear_requested;
    uint8_t accum_clear_requested;
    uint8_t motion_clear_requested;

    if ((reinitialize_pending == 0U) && (axis_config_pending == 0U) && (i2c_error_clear_pending == 0U) &&
        (accum_clear_pending == 0U) && (motion_clear_pending == 0U)) {
        return;
    }

    __disable_irq();
    axis_request_pending    = axis_config_pending;
    requested_axis_config   = axis_config_request;
    axis_config_pending     = 0U;
    reinitialize_requested  = reinitialize_pending;
    reinitialize_pending    = 0U;
    error_clear_requested   = i2c_error_clear_pending;
    i2c_error_clear_pending = 0U;
    accum_clear_requested   = accum_clear_pending;
    accum_clear_pending     = 0U;
    motion_clear_requested  = motion_clear_pending;
    motion_clear_pending    = 0U;
    __enable_irq();

    if (error_clear_requested != 0U) {
        mixo_ots_clear_i2c_error_count();
    }

    if (accum_clear_requested != 0U) {
        mixo_ots_clear_accum();
    }

    if (motion_clear_requested != 0U) {
        mixo_ots_clear_motion_delta();
    }

    if (axis_request_pending != 0U) {
        if ((sensor_initialized == 0U) || (reinitialize_requested != 0U)) {
            if ((reinitialize_requested == 0U) && (axis_config != requested_axis_config)) {
                mixo_ots_clear_motion_data();
            }
            axis_config = requested_axis_config;
        } else if (axis_config != requested_axis_config) {
            config_result = mixo_ots_apply_axis_config(requested_axis_config);
            if (config_result == MIXO_OTS_CONFIG_OK) {
                axis_config = requested_axis_config;
                mixo_ots_clear_motion_data();
            } else if (config_result == MIXO_OTS_CONFIG_STATE_UNKNOWN) {
                sensor_initialized = 0U;
                sensor_init_status = MIXO_OTS_INIT_FAILED;
                polling_active     = 0U;
                irq_pending        = 0U;
                mixo_ots_mot_exti_deinit();
                mixo_ots_clear_motion_data();
            }
        }
    }

    if (reinitialize_requested == 0U) {
        return;
    }

    sensor_init_status = MIXO_OTS_INITING;
    sensor_initialized = 0U;
    polling_active     = 0U;
    irq_pending        = 0U;
    idle_poll_count    = 0U;
    last_poll_ms       = 0U;
    mixo_ots_mot_exti_deinit();
    mixo_ots_clear_motion_data();
    initialized        = mixo_ots_try_init();
    sensor_initialized = initialized;
    sensor_init_status = (initialized != 0U) ? MIXO_OTS_INIT_SUCCESS : MIXO_OTS_INIT_FAILED;
}

bool mixo_hal_get_mot_level(void)
{
    return (HAL_GPIO_ReadPin(MOT_GPIO_Port, MOT_Pin) == GPIO_PIN_RESET) ? true : false;
}

void mixo_ots_mot_exti_init(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(MOT_Pin);
    EXTI->IMR1 |= MOT_Pin;
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0U, 0U);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void mixo_ots_mot_exti_deinit(void)
{
    EXTI->IMR1 &= (uint32_t)(~MOT_Pin);
    __HAL_GPIO_EXTI_CLEAR_IT(MOT_Pin);
}

void mixo_ots_app_init(void)
{
    uint8_t retry;

    mixo_ots_mot_exti_deinit();
    mixo_ots_power_on();
    mixo_ots_set_slave_id(MIXO_OTS_I2C_ADDR_7BIT);

    polling_active                    = 0U;
    irq_pending                       = 0U;
    idle_poll_count                   = 0U;
    mixo_ots_data.ready_generation    = 0U;
    mixo_ots_data.dx_accum_generation = 0U;
    mixo_ots_data.dy_accum_generation = 0U;
    mixo_ots_data.dx_accum            = 0;
    mixo_ots_data.dy_accum            = 0;
    mixo_ots_update_data(0, 0, 0U);

    sensor_initialized      = 0U;
    sensor_init_status      = MIXO_OTS_INITING;
    reinitialize_pending    = 0U;
    axis_config_pending     = 0U;
    i2c_error_clear_pending = 0U;
    accum_clear_pending     = 0U;
    motion_clear_pending    = 0U;
    axis_config             = 0U;
    for (retry = 0U; retry < MIXO_OTS_INIT_RETRY_COUNT; retry++) {
        sensor_initialized = mixo_ots_try_init();
        if (sensor_initialized != 0U) {
            sensor_init_status = MIXO_OTS_INIT_SUCCESS;
            break;
        }
        if ((retry + 1U) < MIXO_OTS_INIT_RETRY_COUNT) {
            HAL_Delay(MIXO_OTS_INIT_RETRY_DELAY_MS);
        }
    }
    if (sensor_initialized == 0U) {
        sensor_init_status = MIXO_OTS_INIT_FAILED;
    }
}

uint8_t mixo_ots_get_init_status(void)
{
    return sensor_init_status;
}

void mixo_ots_request_reinitialize(void)
{
    reinitialize_pending = 1U;
    sensor_init_status   = MIXO_OTS_INITING;
}

uint8_t mixo_ots_get_axis_config(void)
{
    return axis_config;
}

void mixo_ots_request_axis_config(uint8_t config)
{
    if ((config & (uint8_t)~0x07U) == 0U) {
        axis_config_request = config;
        axis_config_pending = 1U;
    }
}

uint8_t mixo_ots_app_get_i2c_error_count(void)
{
    return mixo_ots_get_i2c_error_count();
}

void mixo_ots_request_i2c_error_count_clear(void)
{
    i2c_error_clear_pending = 1U;
}

void mixo_ots_request_clear_accum(void)
{
    accum_clear_pending = 1U;
}

void mixo_ots_request_clear_motion(void)
{
    motion_clear_pending = 1U;
}

void mixo_ots_app_poll(void)
{
    uint32_t now        = mixo_hal_get_tick_ms();
    uint8_t mot_status  = 0U;
    raw_data_t raw_data = {0, 0};
    int ret;

    mixo_ots_pending_service();

    if (sensor_initialized == 0U) {
        return;
    }

    if (irq_pending != 0U) {
        irq_pending = 0U;
        mixo_ots_start_polling();
    }

    if (polling_active == 0U) {
        return;
    }

    if ((now - last_poll_ms) < MIXO_OTS_POLL_PERIOD_MS) {
        return;
    }
    last_poll_ms = now;

    ret = mixo_ots_get_mot_status(&mot_status);
    if (ret != 0) {
        return;
    }

    if ((mot_status & MIXO_OTS_MOTION_FLAG) == MIXO_OTS_MOTION_FLAG) {
        idle_poll_count = 0U;
        ret             = mixo_ots_get_raw_data(&raw_data);
        if ((ret == 0) && ((mot_status & MIXO_OTS_OVERFLOW_MASK) == 0U)) {
            mixo_ots_update_data(raw_data.dx, raw_data.dy, 1U);
        }
    } else {
        idle_poll_count++;
        if (idle_poll_count >= MIXO_OTS_IDLE_POLL_LIMIT) {
            if (mixo_ots_forced_clear_int() != 0) {
                return;
            }

            mixo_ots_mot_exti_init();
            if (mixo_hal_get_mot_level()) {
                mixo_ots_mot_exti_deinit();
                idle_poll_count = 0U;
                return;
            }

            polling_active = 0U;
        }
    }
}

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == MOT_Pin && mixo_hal_get_mot_level()) {
        mixo_ots_mot_exti_deinit();
        irq_pending = 1U;
    }
}
