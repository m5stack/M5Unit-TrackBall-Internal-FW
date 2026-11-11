/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_REG_H__
#define __USER_I2C_REG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Trackball motion data, int16 little-endian (0x00-0x03).
#define REG_TRACKBALL_DATA_START (0x00U)
#define REG_TRACKBALL_DX_LOW     (0x00U)
#define REG_TRACKBALL_DX_HIGH    (0x01U)
#define REG_TRACKBALL_DY_LOW     (0x02U)
#define REG_TRACKBALL_DY_HIGH    (0x03U)
#define REG_TRACKBALL_DATA_END   REG_TRACKBALL_DY_HIGH

// Accumulated motion data, int32 little-endian, explicitly cleared by 0x41/0x01 (0x06-0x0D).
#define REG_X_ACCUM_START    (0x06U)
#define REG_X_ACCUM_END      (0x09U)
#define REG_Y_ACCUM_START    (0x0AU)
#define REG_Y_ACCUM_END      (0x0DU)
#define REG_ACCUM_DATA_START REG_X_ACCUM_START
#define REG_ACCUM_DATA_END   REG_Y_ACCUM_END

// Input status and key event (0x10-0x12).
#define REG_TRACKBALL_READY    (0x10U)
#define REG_KEY_STATE          (0x11U)
#define REG_KEY_EVENT          (0x12U)
#define REG_INPUT_STATUS_START REG_TRACKBALL_READY
#define REG_INPUT_STATUS_END   REG_KEY_EVENT

// Sensor initialization and control registers.
#define REG_SENSOR_INIT_STATUS (0x13U)
#define REG_SENSOR_REINIT      (0x40U)
#define SENSOR_INIT_SUCCESS    (0x00U)
#define SENSOR_INIT_FAILED     (0x01U)
#define SENSOR_INITING         (0x02U)
#define SENSOR_REINIT_COMMAND  (0x01U)

// Explicit data clear command register, write-only.
#define REG_CLEAR_DATA       (0x41U)
#define CLEAR_ACCUM_COMMAND  (0x01U)
#define CLEAR_MOTION_COMMAND (0x02U)

// User-level axis configuration (0x50).
#define REG_AXIS_CONFIG        (0x50U)
#define AXIS_CONFIG_X_INVERT   (0x01U)
#define AXIS_CONFIG_Y_INVERT   (0x02U)
#define AXIS_CONFIG_XY_SWAP    (0x04U)
#define AXIS_CONFIG_VALID_MASK (AXIS_CONFIG_X_INVERT | AXIS_CONFIG_Y_INVERT | AXIS_CONFIG_XY_SWAP)

// Internal sensor I2C error count (0x51).
#define REG_I2C_ERROR_COUNT (0x51U)

// LED PWM duty cycle (0x20).
#define REG_LED_PWM_DUTY (0x20U)

// Key timing configuration, uint16 little-endian in milliseconds (0x30-0x35).
#define REG_KEY_DEBOUNCE_LOW      (0x30U)
#define REG_KEY_DEBOUNCE_HIGH     (0x31U)
#define REG_KEY_DOUBLE_CLICK_LOW  (0x32U)
#define REG_KEY_DOUBLE_CLICK_HIGH (0x33U)
#define REG_KEY_LONG_PRESS_LOW    (0x34U)
#define REG_KEY_LONG_PRESS_HIGH   (0x35U)
#define REG_KEY_CONFIG_START      REG_KEY_DEBOUNCE_LOW
#define REG_KEY_CONFIG_END        REG_KEY_LONG_PRESS_HIGH

// MCU unique device ID, read-only (0xE0-0xEB).
#define REG_UID_BYTE0  (0xE0U)
#define REG_UID_BYTE1  (0xE1U)
#define REG_UID_BYTE2  (0xE2U)
#define REG_UID_BYTE3  (0xE3U)
#define REG_UID_BYTE4  (0xE4U)
#define REG_UID_BYTE5  (0xE5U)
#define REG_UID_BYTE6  (0xE6U)
#define REG_UID_BYTE7  (0xE7U)
#define REG_UID_BYTE8  (0xE8U)
#define REG_UID_BYTE9  (0xE9U)
#define REG_UID_BYTE10 (0xEAU)
#define REG_UID_BYTE11 (0xEBU)
#define REG_UID_START  REG_UID_BYTE0
#define REG_UID_END    REG_UID_BYTE11

// Firmware update and device information (0xFD-0xFF).
#define REG_IAP_UPDATE_ADDR   (0xFDU)
#define REG_SW_VER            (0xFEU)
#define REG_I2C_ADDR          (0xFFU)
#define REG_DEVICE_INFO_START REG_SW_VER
#define REG_DEVICE_INFO_END   REG_I2C_ADDR
#define USER_I2C_SW_VERSION   (0x01U)

#ifdef __cplusplus
}
#endif

#endif /* __USER_I2C_REG_H__ */
