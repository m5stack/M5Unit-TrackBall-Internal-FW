/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_H__
#define __USER_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdio.h"
#include "string.h"
#include "i2c.h"
#include "user_i2c_callback.h"

// Macro to suppress unused variable warnings / 抑制未使用变量警告的宏
#define UNUSED(X) (void)X

// Size of the I2C receive/transmit buffers / I2C 接收/发送缓冲区的大小
#define I2C_RECEIVE_BUFFER_LEN (600)
#define I2C_INVALID_DATA       (0xFFU)

// Default 7-bit I2C slave address / 默认 7位 I2C 从机地址
#define USER_I2C_DEFAULT_ADDR (0x3F)
#define USER_I2C_MIN_ADDR     (0x08U)
#define USER_I2C_MAX_ADDR     (0x77U)

// Set while an I2C write transaction is waiting for STOP.
extern volatile uint32_t i2c_stop_timeout_flag;
extern volatile uint32_t i2c_stop_timeout_counter;
extern volatile uint32_t i2c2_recovery_pending;

/**
 * @brief Initialize I2C1 peripheral in Slave mode.
 *        在从机模式下初始化 I2C1 外设。
 */
void user_i2c_init(void);

/**
 * @brief Enable I2C1 interrupts (ADDR, NACK, ERR, STOP).
 *        启用 I2C1 中断 (ADDR, NACK, ERR, STOP)。
 */
void i2c2_it_enable(void);

/**
 * @brief Disable I2C1 interrupts.
 *        禁用 I2C1 中断。
 */
void i2c2_it_disable(void);

/**
 * @brief Recover I2C2 after an incomplete transaction.
 *        未完成事务超时后恢复 I2C2。
 */
void i2c2_recover(void);

/**
 * @brief Prepare data to be sent to the Master.
 *        准备发送给主机的数据。
 * @param tx_ptr Pointer to data. / 数据指针。
 * @param len Length of data. / 数据长度。
 */
void i2c2_set_send_data(const volatile uint8_t *tx_ptr, uint16_t len);

/**
 * @brief Set the I2C Slave Address (Software variable).
 *        设置 I2C 从机地址（软件变量）。
 * @param addr 7-bit address (e.g., 0x25). / 7位地址 (例如 0x25)。
 */
void set_i2c_slave_address(uint8_t addr);

#ifdef __cplusplus
}
#endif
#endif /*__USER_I2C_H__ */
