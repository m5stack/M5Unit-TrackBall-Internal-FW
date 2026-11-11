/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_FLASH_H__
#define __USER_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "stm32g0xx_hal.h"

/* The last 2-KB page is reserved for persistent product configuration. */
#define USER_FLASH_CONFIG_ADDR (0x08007800UL)
#define USER_FLASH_CONFIG_PAGE ((USER_FLASH_CONFIG_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE)
#define USER_FLASH_ERASED_BYTE (0xFFU)

/**
 * @brief Load the latest valid I2C address from Flash.
 * @param address Destination for the stored 7-bit address.
 * @retval 1 if a valid record was found, otherwise 0.
 */
uint8_t user_flash_i2c_addr_read(uint8_t *address);

/**
 * @brief Append an I2C address record to Flash.
 * @retval 1 if the address was stored and verified, otherwise 0.
 */
uint8_t user_flash_i2c_addr_write(uint8_t address);

#ifdef __cplusplus
}
#endif

#endif /* __USER_FLASH_H__ */
