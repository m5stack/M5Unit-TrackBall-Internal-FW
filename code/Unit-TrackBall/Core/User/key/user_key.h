/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_KEY_H__
#define __USER_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define USER_KEY_DEBOUNCE_DEFAULT_MS     (20U)
#define USER_KEY_DOUBLE_CLICK_DEFAULT_MS (300U)
#define USER_KEY_LONG_PRESS_DEFAULT_MS   (1000U)

typedef enum {
    USER_KEY_EVENT_NONE         = 0x00U,
    USER_KEY_EVENT_SINGLE_CLICK = 0x01U,
    USER_KEY_EVENT_DOUBLE_CLICK = 0x02U,
    USER_KEY_EVENT_LONG_PRESS   = 0x03U,
} user_key_event_t;

typedef struct {
    uint16_t debounce_ms;
    uint16_t double_click_ms;
    uint16_t long_press_ms;
} user_key_config_t;

void user_key_init(void);
void user_key_poll(void);
uint8_t user_key_get_state(void);
uint8_t user_key_get_event(void);
uint8_t user_key_peek_event(uint8_t *generation);
void user_key_clear_event(uint8_t event, uint8_t generation);
uint8_t user_key_configure(const user_key_config_t *config);
void user_key_get_config(user_key_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* __USER_KEY_H__ */
