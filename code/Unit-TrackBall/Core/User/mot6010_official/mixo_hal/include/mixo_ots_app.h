#pragma once

#include <stdint.h>

#define MIXO_OTS_INIT_SUCCESS (0x00U)
#define MIXO_OTS_INIT_FAILED  (0x01U)
#define MIXO_OTS_INITING      (0x02U)

typedef struct {
    int16_t dx;
    int16_t dy;
    int32_t dx_accum;
    int32_t dy_accum;
    uint8_t ready;
    uint8_t ready_generation;
    uint32_t dx_accum_generation;
    uint32_t dy_accum_generation;
} mixo_ots_data_t;

extern volatile mixo_ots_data_t mixo_ots_data;

void mixo_ots_app_init(void);
void mixo_ots_app_poll(void);
uint8_t mixo_ots_get_init_status(void);
void mixo_ots_request_reinitialize(void);
uint8_t mixo_ots_get_axis_config(void);
void mixo_ots_request_axis_config(uint8_t config);
uint8_t mixo_ots_app_get_i2c_error_count(void);
void mixo_ots_request_i2c_error_count_clear(void);
void mixo_ots_request_clear_accum(void);
void mixo_ots_request_clear_motion(void);
void mixo_ots_mot_exti_init(void);
void mixo_ots_mot_exti_deinit(void);
