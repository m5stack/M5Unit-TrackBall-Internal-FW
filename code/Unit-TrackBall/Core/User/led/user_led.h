/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_LED_H__
#define __USER_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define USER_LED_PWM_DUTY_MIN (0U)
#define USER_LED_PWM_DUTY_MAX (100U)

uint8_t user_led_init(void);
void user_led_set_duty(uint8_t duty);
uint8_t user_led_get_duty(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_LED_H__ */
