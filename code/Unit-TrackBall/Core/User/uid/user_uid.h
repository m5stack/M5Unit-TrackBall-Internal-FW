/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_UID_H__
#define __USER_UID_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define USER_UID_LENGTH (12U)

extern volatile uint8_t uid_data[USER_UID_LENGTH];

void user_uid_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_UID_H__ */
