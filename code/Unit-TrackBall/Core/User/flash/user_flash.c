/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_flash.h"

static uint8_t user_flash_read_byte(void)
{
    return *((const volatile uint8_t *)USER_FLASH_CONFIG_ADDR);
}

uint8_t user_flash_i2c_addr_read(uint8_t *address)
{
    if (address == NULL) {
        return 0U;
    }

    *address = user_flash_read_byte();
    if (*address == USER_FLASH_ERASED_BYTE) {
        return 0U;
    }
    return 1U;
}

uint8_t user_flash_i2c_addr_write(uint8_t address)
{
    FLASH_EraseInitTypeDef erase = {0};
    HAL_StatusTypeDef status     = HAL_ERROR;
    uint32_t page_error          = 0U;
    uint32_t primask             = __get_PRIMASK();
    uint8_t result               = 0U;
    /* The STM32G0 programs a double-word. Keep the remaining bytes erased. */
    uint64_t data = 0xFFFFFFFFFFFFFF00ULL | (uint64_t)address;

    /* Match the known-good STM32G0 sequence: no interrupt may execute from
       Flash while the configuration page is being erased/programmed. */
    __disable_irq();

    if (HAL_FLASH_Unlock() != HAL_OK) {
        goto restore_interrupts;
    }

    FLASH->SR       = FLASH_SR_CLEAR;
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.Banks     = FLASH_BANK_1;
    erase.Page      = USER_FLASH_CONFIG_PAGE;
    erase.NbPages   = 1U;
    status          = HAL_FLASHEx_Erase(&erase, &page_error);
    if (status == HAL_OK) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, USER_FLASH_CONFIG_ADDR, data);
    }

    (void)HAL_FLASH_Lock();
    if ((status == HAL_OK) && (user_flash_read_byte() == address)) {
        result = 1U;
    }

restore_interrupts:
    __set_PRIMASK(primask);
    return result;
}
