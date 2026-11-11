/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_uid.h"

#include "main.h"

volatile uint8_t uid_data[USER_UID_LENGTH];

void user_uid_init(void)
{
    uint32_t uid0 = HAL_GetUIDw0();
    uint32_t uid1 = HAL_GetUIDw1();
    uint32_t uid2 = HAL_GetUIDw2();

    uid_data[0]  = (uint8_t)(uid0 & 0xFFU);
    uid_data[1]  = (uint8_t)((uid0 >> 8) & 0xFFU);
    uid_data[2]  = (uint8_t)((uid0 >> 16) & 0xFFU);
    uid_data[3]  = (uint8_t)((uid0 >> 24) & 0xFFU);
    uid_data[4]  = (uint8_t)(uid1 & 0xFFU);
    uid_data[5]  = (uint8_t)((uid1 >> 8) & 0xFFU);
    uid_data[6]  = (uint8_t)((uid1 >> 16) & 0xFFU);
    uid_data[7]  = (uint8_t)((uid1 >> 24) & 0xFFU);
    uid_data[8]  = (uint8_t)(uid2 & 0xFFU);
    uid_data[9]  = (uint8_t)((uid2 >> 8) & 0xFFU);
    uid_data[10] = (uint8_t)((uid2 >> 16) & 0xFFU);
    uid_data[11] = (uint8_t)((uid2 >> 24) & 0xFFU);
}
