/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "user_key.h"

#include <stddef.h>

#include "main.h"

static user_key_config_t key_config;
static uint32_t raw_change_ms;
static uint32_t press_start_ms;
static uint32_t first_release_ms;
static uint8_t raw_pressed;
static volatile uint8_t stable_pressed;
static uint8_t click_pending;
static uint8_t second_click_active;
static uint8_t long_press_triggered;
static volatile uint8_t key_event;
static volatile uint8_t key_event_generation;

static uint8_t user_key_read_pin(void)
{
    return (HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

static void user_key_set_event(user_key_event_t event)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    key_event = (uint8_t)event;
    key_event_generation++;
    if (primask == 0U) {
        __enable_irq();
    }
}

void user_key_init(void)
{
    uint8_t pressed = user_key_read_pin();

    key_config.debounce_ms     = USER_KEY_DEBOUNCE_DEFAULT_MS;
    key_config.double_click_ms = USER_KEY_DOUBLE_CLICK_DEFAULT_MS;
    key_config.long_press_ms   = USER_KEY_LONG_PRESS_DEFAULT_MS;

    raw_change_ms        = HAL_GetTick();
    press_start_ms       = raw_change_ms;
    first_release_ms     = 0U;
    raw_pressed          = pressed;
    stable_pressed       = pressed;
    click_pending        = 0U;
    second_click_active  = 0U;
    long_press_triggered = 0U;
    key_event            = USER_KEY_EVENT_NONE;
    key_event_generation = 0U;
}

void user_key_poll(void)
{
    uint32_t now    = HAL_GetTick();
    uint8_t pressed = user_key_read_pin();
    user_key_config_t config;

    user_key_get_config(&config);

    if (pressed != raw_pressed) {
        raw_pressed   = pressed;
        raw_change_ms = now;
    }

    if ((stable_pressed != raw_pressed) && ((uint32_t)(now - raw_change_ms) >= config.debounce_ms)) {
        stable_pressed = raw_pressed;

        if (stable_pressed != 0U) {
            press_start_ms       = now;
            long_press_triggered = 0U;
            if ((click_pending != 0U) && ((uint32_t)(now - first_release_ms) <= config.double_click_ms)) {
                second_click_active = 1U;
            } else {
                if (click_pending != 0U) {
                    user_key_set_event(USER_KEY_EVENT_SINGLE_CLICK);
                }
                click_pending       = 0U;
                second_click_active = 0U;
            }
        } else if (long_press_triggered == 0U) {
            if (second_click_active != 0U) {
                click_pending       = 0U;
                second_click_active = 0U;
                user_key_set_event(USER_KEY_EVENT_DOUBLE_CLICK);
            } else {
                click_pending    = 1U;
                first_release_ms = now;
            }
        }
    }

    if ((stable_pressed != 0U) && (long_press_triggered == 0U) &&
        ((uint32_t)(now - press_start_ms) >= config.long_press_ms)) {
        long_press_triggered = 1U;
        click_pending        = 0U;
        second_click_active  = 0U;
        user_key_set_event(USER_KEY_EVENT_LONG_PRESS);
    }

    if ((click_pending != 0U) && (second_click_active == 0U) &&
        ((uint32_t)(now - first_release_ms) > config.double_click_ms)) {
        click_pending = 0U;
        user_key_set_event(USER_KEY_EVENT_SINGLE_CLICK);
    }
}

uint8_t user_key_get_state(void)
{
    return stable_pressed;
}

uint8_t user_key_get_event(void)
{
    uint32_t primask = __get_PRIMASK();
    uint8_t event;

    __disable_irq();
    event = key_event;
    if (event != USER_KEY_EVENT_NONE) {
        key_event = USER_KEY_EVENT_NONE;
        key_event_generation++;
    }
    if (primask == 0U) {
        __enable_irq();
    }
    return event;
}

uint8_t user_key_peek_event(uint8_t *generation)
{
    uint32_t primask = __get_PRIMASK();
    uint8_t event;

    __disable_irq();
    event = key_event;
    if (generation != NULL) {
        *generation = key_event_generation;
    }
    if (primask == 0U) {
        __enable_irq();
    }
    return event;
}

void user_key_clear_event(uint8_t event, uint8_t generation)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    if ((event != USER_KEY_EVENT_NONE) && (key_event == event) && (key_event_generation == generation)) {
        key_event = USER_KEY_EVENT_NONE;
        key_event_generation++;
    }
    if (primask == 0U) {
        __enable_irq();
    }
}

uint8_t user_key_configure(const user_key_config_t *config)
{
    uint32_t primask;

    if ((config == NULL) || (config->debounce_ms == 0U) || (config->double_click_ms <= config->debounce_ms) ||
        (config->long_press_ms <= config->double_click_ms)) {
        return 0U;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    key_config = *config;
    if (primask == 0U) {
        __enable_irq();
    }
    return 1U;
}

void user_key_get_config(user_key_config_t *config)
{
    uint32_t primask;

    if (config == NULL) {
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    *config = key_config;
    if (primask == 0U) {
        __enable_irq();
    }
}
