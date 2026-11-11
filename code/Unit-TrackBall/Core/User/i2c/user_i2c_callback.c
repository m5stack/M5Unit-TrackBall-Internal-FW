/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_i2c_callback.h"
#include "mixo_ots_app.h"
#include "user_flash.h"
#include "user_key.h"
#include "user_led.h"
#include "user_uid.h"

#define I2C_STOP_TIMEOUT_MS       (500U)
#define I2C_STOP_TIMEOUT_CHECK_MS (10U)
#define I2C_STOP_TIMEOUT_COUNT    (I2C_STOP_TIMEOUT_MS / I2C_STOP_TIMEOUT_CHECK_MS)

static uint32_t i2c_stop_timeout_delay;
static volatile uint8_t i2c_addr_update_pending;
static volatile uint8_t i2c_addr_pending;
static volatile uint8_t ready_read_pending;
static volatile uint8_t ready_read_value;
static volatile uint8_t ready_read_generation;
static volatile uint8_t key_event_read_pending;
static volatile uint8_t key_event_read_value;
static volatile uint8_t key_event_read_offset;
static volatile uint8_t key_event_read_generation;

volatile uint8_t i2c_addr_reg = USER_I2C_DEFAULT_ADDR;  // Example register address for demonstration

extern volatile mixo_ots_data_t mixo_ots_data;

static void key_config_to_bytes(const user_key_config_t *config, uint8_t *data)
{
    data[0] = (uint8_t)(config->debounce_ms & 0xFFU);
    data[1] = (uint8_t)((config->debounce_ms >> 8) & 0xFFU);
    data[2] = (uint8_t)(config->double_click_ms & 0xFFU);
    data[3] = (uint8_t)((config->double_click_ms >> 8) & 0xFFU);
    data[4] = (uint8_t)(config->long_press_ms & 0xFFU);
    data[5] = (uint8_t)((config->long_press_ms >> 8) & 0xFFU);
}

static void key_config_from_bytes(user_key_config_t *config, const uint8_t *data)
{
    config->debounce_ms     = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
    config->double_click_ms = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
    config->long_press_ms   = (uint16_t)data[4] | ((uint16_t)data[5] << 8);
}

static void key_config_write(const uint8_t *rx_data, uint16_t len)
{
    user_key_config_t config;
    uint8_t data[6];
    uint8_t offset    = rx_data[0] - REG_KEY_CONFIG_START;
    uint16_t data_len = len - 1U;
    uint16_t index;

    if (data_len > (uint16_t)(sizeof(data) - offset)) {
        return;
    }

    user_key_get_config(&config);
    key_config_to_bytes(&config, data);
    for (index = 0U; index < data_len; index++) {
        data[offset + index] = rx_data[index + 1U];
    }
    key_config_from_bytes(&config, data);
    (void)user_key_configure(&config);
}

/* Write-only command group 0x40 ~ 0x41: consecutive writes from any offset. */
static void command_config_write(const uint8_t *rx_data, uint16_t len)
{
    uint16_t data_len = len - 1U;
    uint16_t index;

    if (data_len > (uint16_t)(REG_CLEAR_DATA - rx_data[0] + 1U)) {
        return;
    }
    for (index = 0U; index < data_len; index++) {
        uint8_t reg   = rx_data[0] + (uint8_t)index;
        uint8_t value = rx_data[index + 1U];

        if (reg == REG_SENSOR_REINIT) {
            if (value == SENSOR_REINIT_COMMAND) {
                mixo_ots_request_reinitialize();
            }
        } else if (reg == REG_CLEAR_DATA) {
            if (value == CLEAR_ACCUM_COMMAND) {
                mixo_ots_request_clear_accum();
            } else if (value == CLEAR_MOTION_COMMAND) {
                mixo_ots_request_clear_motion();
            }
        }
    }
}

/* R/W group 0x50 ~ 0x51: consecutive writes from any offset. */
static void control_config_write(const uint8_t *rx_data, uint16_t len)
{
    uint16_t data_len = len - 1U;
    uint16_t index;

    if (data_len > (uint16_t)(REG_I2C_ERROR_COUNT - rx_data[0] + 1U)) {
        return;
    }
    for (index = 0U; index < data_len; index++) {
        uint8_t reg   = rx_data[0] + (uint8_t)index;
        uint8_t value = rx_data[index + 1U];

        if (reg == REG_AXIS_CONFIG) {
            if ((value & (uint8_t)~AXIS_CONFIG_VALID_MASK) == 0U) {
                mixo_ots_request_axis_config(value);
            }
        } else if (reg == REG_I2C_ERROR_COUNT) {
            if (value == 0U) {
                mixo_ots_request_i2c_error_count_clear();
            }
        }
    }
}

static void put_i32_le(uint8_t *data, int32_t value)
{
    uint32_t raw = (uint32_t)value;
    data[0]      = (uint8_t)(raw & 0xFFU);
    data[1]      = (uint8_t)((raw >> 8) & 0xFFU);
    data[2]      = (uint8_t)((raw >> 16) & 0xFFU);
    data[3]      = (uint8_t)((raw >> 24) & 0xFFU);
}

void i2c_address_init(void)
{
    uint8_t stored_addr;

    if ((user_flash_i2c_addr_read(&stored_addr) != 0U) && (stored_addr >= USER_I2C_MIN_ADDR) &&
        (stored_addr <= USER_I2C_MAX_ADDR)) {
        i2c_addr_reg = stored_addr;
    } else {
        i2c_addr_reg = USER_I2C_DEFAULT_ADDR;
    }
}

static void i2c_address_write(uint8_t addr)
{
    if ((addr >= USER_I2C_MIN_ADDR) && (addr <= USER_I2C_MAX_ADDR) && (addr != i2c_addr_reg)) {
        i2c_addr_pending        = addr;
        i2c_addr_update_pending = 1U;
    }
}

void i2c_address_update_handler(void)
{
    uint8_t new_addr;

    if (i2c_addr_update_pending == 0U) {
        return;
    }

    NVIC_DisableIRQ(I2C2_IRQn);
    if ((i2c_addr_update_pending == 0U) || (LL_I2C_IsActiveFlag_BUSY(I2C2) != 0U)) {
        NVIC_EnableIRQ(I2C2_IRQn);
        return;
    }

    new_addr                = i2c_addr_pending;
    i2c_addr_update_pending = 0U;

    i2c2_it_disable();
    LL_I2C_Disable(I2C2);

    if (user_flash_i2c_addr_write(new_addr) != 0U) {
        i2c_addr_reg = new_addr;
    }

    LL_I2C_DeInit(I2C2);
    user_i2c_init();
    NVIC_ClearPendingIRQ(I2C2_IRQn);
    i2c2_it_enable();
}

/**
 * @brief Main Callback for I2C Transaction Completion.
 *        I2C 事务完成的主回调函数。
 */
void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    uint8_t tx_buf[64];
    uint8_t invalid_data = I2C_INVALID_DATA;

    ready_read_pending     = 0U;
    key_event_read_pending = 0U;
    i2c2_set_send_data(&invalid_data, 1U);

    if (len == 1U) {
        uint8_t reg = rx_data[0];

        if ((reg >= REG_X_ACCUM_START) && (reg <= REG_ACCUM_DATA_END)) {
            uint8_t offset   = (uint8_t)(reg - REG_X_ACCUM_START);
            uint8_t send_len = (uint8_t)(REG_ACCUM_DATA_END - reg + 1U);

            put_i32_le(&tx_buf[0], mixo_ots_data.dx_accum);
            put_i32_le(&tx_buf[4], mixo_ots_data.dy_accum);
            i2c2_set_send_data(&tx_buf[offset], send_len);
        } else if ((reg >= REG_TRACKBALL_DATA_START) && (reg <= REG_TRACKBALL_DATA_END)) {
            uint8_t offset = reg - REG_TRACKBALL_DATA_START;
            tx_buf[0]      = (uint8_t)(mixo_ots_data.dx & 0xFF);
            tx_buf[1]      = (uint8_t)((mixo_ots_data.dx >> 8) & 0xFF);
            tx_buf[2]      = (uint8_t)(mixo_ots_data.dy & 0xFF);
            tx_buf[3]      = (uint8_t)((mixo_ots_data.dy >> 8) & 0xFF);
            i2c2_set_send_data(&tx_buf[offset], (uint16_t)(REG_TRACKBALL_DATA_END - reg + 1U));
        } else if ((reg >= REG_INPUT_STATUS_START) && (reg <= REG_INPUT_STATUS_END)) {
            tx_buf[0] = mixo_ots_data.ready;
            tx_buf[1] = user_key_get_state();
            tx_buf[2] = user_key_peek_event((uint8_t *)&key_event_read_generation);
            if (reg == REG_TRACKBALL_READY) {
                ready_read_value      = tx_buf[0];
                ready_read_generation = mixo_ots_data.ready_generation;
                ready_read_pending    = 1U;
            }
            key_event_read_value   = tx_buf[2];
            key_event_read_offset  = REG_KEY_EVENT - reg;
            key_event_read_pending = 1U;
            i2c2_set_send_data(&tx_buf[reg - REG_INPUT_STATUS_START], (uint16_t)(REG_INPUT_STATUS_END - reg + 1U));
        } else if (reg == REG_LED_PWM_DUTY) {
            tx_buf[0] = user_led_get_duty();
            i2c2_set_send_data(&tx_buf[0], 1U);
        } else if ((reg >= REG_KEY_CONFIG_START) && (reg <= REG_KEY_CONFIG_END)) {
            user_key_config_t config;
            uint8_t offset = reg - REG_KEY_CONFIG_START;

            user_key_get_config(&config);
            key_config_to_bytes(&config, tx_buf);
            i2c2_set_send_data(&tx_buf[offset], (uint16_t)(REG_KEY_CONFIG_END - reg + 1U));
        } else if ((reg >= REG_UID_START) && (reg <= REG_UID_END)) {
            uint8_t offset = reg - REG_UID_START;
            i2c2_set_send_data(&uid_data[offset], (uint16_t)(USER_UID_LENGTH - offset));
        } else if ((reg >= REG_DEVICE_INFO_START) && (reg <= REG_DEVICE_INFO_END)) {
            tx_buf[0] = USER_I2C_SW_VERSION;
            tx_buf[1] = i2c_addr_reg;
            i2c2_set_send_data(&tx_buf[reg - REG_DEVICE_INFO_START], (uint16_t)(REG_DEVICE_INFO_END - reg + 1U));
        } else if (reg == REG_SENSOR_INIT_STATUS) {
            tx_buf[0] = mixo_ots_get_init_status();
            i2c2_set_send_data(&tx_buf[0], 1U);
        } else if ((reg >= REG_AXIS_CONFIG) && (reg <= REG_I2C_ERROR_COUNT)) {
            uint8_t offset = reg - REG_AXIS_CONFIG;
            tx_buf[0]      = mixo_ots_get_axis_config();
            tx_buf[1]      = mixo_ots_app_get_i2c_error_count();
            i2c2_set_send_data(&tx_buf[offset], (uint16_t)(REG_I2C_ERROR_COUNT - reg + 1U));
        }
    } else if (len >= 2U) {
        if ((rx_data[0] >= REG_KEY_CONFIG_START) && (rx_data[0] <= REG_KEY_CONFIG_END)) {
            key_config_write(rx_data, len);
        } else if ((rx_data[0] >= REG_SENSOR_REINIT) && (rx_data[0] <= REG_CLEAR_DATA)) {
            command_config_write(rx_data, len);
        } else if ((rx_data[0] >= REG_AXIS_CONFIG) && (rx_data[0] <= REG_I2C_ERROR_COUNT)) {
            control_config_write(rx_data, len);
        } else if ((len == 2U) && (rx_data[0] == REG_LED_PWM_DUTY)) {
            user_led_set_duty(rx_data[1]);
        } else if ((len == 2U) && (rx_data[0] == REG_I2C_ADDR)) {
            i2c_address_write(rx_data[1]);
        } else if ((len == 2U) && (rx_data[0] == REG_IAP_UPDATE_ADDR) && (rx_data[1] != 0U)) {
            NVIC_SystemReset();
        }
    }
}

void Slave_Transmit_Complete_Callback(uint16_t transmitted_len)
{
    if ((ready_read_pending != 0U) && (transmitted_len != 0U) && (ready_read_value != 0U) &&
        (mixo_ots_data.ready_generation == ready_read_generation)) {
        mixo_ots_data.ready = 0U;
    }
    ready_read_pending = 0U;

    if ((key_event_read_pending != 0U) && (transmitted_len > key_event_read_offset)) {
        user_key_clear_event(key_event_read_value, key_event_read_generation);
    }
    key_event_read_pending = 0U;
}

void i2c_transaction_state_reset(void)
{
    ready_read_pending        = 0U;
    ready_read_value          = 0U;
    ready_read_generation     = 0U;
    key_event_read_pending    = 0U;
    key_event_read_value      = 0U;
    key_event_read_offset     = 0U;
    key_event_read_generation = 0U;
}

/**
 * @brief I2C Bus Timeout Handler.
 *        I2C 总线超时处理程序。
 * @note  Called periodically in main loop or systick callback.
 *        Attempts to recover I2C if the line gets stuck.
 *        在主循环或系统滴答回调中周期性调用。
 *        如果线路卡住，尝试恢复 I2C。
 */
void i2c_timeout_handler(void)
{
    uint32_t now;

    if (i2c2_recovery_pending != 0U) {
        NVIC_DisableIRQ(I2C2_IRQn);
        if (i2c2_recovery_pending != 0U) {
            i2c2_recover();
            i2c_stop_timeout_delay = 0U;
        } else {
            NVIC_EnableIRQ(I2C2_IRQn);
        }
        return;
    }

    if (i2c_stop_timeout_flag == 0U) {
        i2c_stop_timeout_counter = 0U;
        i2c_stop_timeout_delay   = 0U;
        return;
    }

    now = HAL_GetTick();
    if ((i2c_stop_timeout_delay != 0U) && ((int32_t)(now - i2c_stop_timeout_delay) < 0)) {
        return;
    }
    i2c_stop_timeout_delay = now + I2C_STOP_TIMEOUT_CHECK_MS;
    i2c_stop_timeout_counter++;

    if (i2c_stop_timeout_counter > I2C_STOP_TIMEOUT_COUNT) {
        NVIC_DisableIRQ(I2C2_IRQn);
        if ((i2c_stop_timeout_flag != 0U) && (i2c_stop_timeout_counter > I2C_STOP_TIMEOUT_COUNT)) {
            i2c2_recover();
            i2c_stop_timeout_delay = 0U;
        } else {
            NVIC_EnableIRQ(I2C2_IRQn);
        }
    }
}
