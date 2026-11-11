/*
 * STM32 hardware-I2C port for the official MixoSense OTS driver.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

uint8_t mixo_gpio_i2c_write_read(uint8_t slave_id, uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer,
                                 size_t read_size);

void mixo_i2c_set_timing_register(uint32_t timing_register);
void mixo_i2c_set_check_count(int count);
