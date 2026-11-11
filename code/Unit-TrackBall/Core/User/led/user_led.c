/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_led.h"

#include "tim.h"

static volatile uint8_t led_pwm_duty;

uint8_t user_led_init(void)
{
    led_pwm_duty = USER_LED_PWM_DUTY_MIN;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0U);

    return (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3) == HAL_OK) ? 1U : 0U;
}

void user_led_set_duty(uint8_t duty)
{
    uint32_t period;
    uint32_t compare;

    if (duty > USER_LED_PWM_DUTY_MAX) {
        duty = USER_LED_PWM_DUTY_MAX;
    }

    period       = __HAL_TIM_GET_AUTORELOAD(&htim1);
    compare      = ((period + 1U) * duty) / USER_LED_PWM_DUTY_MAX;
    led_pwm_duty = duty;
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, compare);
}

uint8_t user_led_get_duty(void)
{
    return led_pwm_duty;
}
