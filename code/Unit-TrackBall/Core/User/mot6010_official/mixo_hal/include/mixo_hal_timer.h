/*
 * Minimal timer API required by the official MixoSense OTS driver.
 */
#pragma once

#include <stdint.h>

void mixo_hal_delay_ms(uint32_t ms);
void mixo_hal_delay_us(uint32_t us);
void mixo_hal_delay_nop(uint32_t count);
uint32_t mixo_hal_get_tick_ms(void);
uint32_t mixo_hal_get_tick_us(void);
