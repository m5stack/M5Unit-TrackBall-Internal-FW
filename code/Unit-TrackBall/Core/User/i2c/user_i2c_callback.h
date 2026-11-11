/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __USER_I2C_CALLBACK_H__
#define __USER_I2C_CALLBACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "user_i2c_reg.h"
#include "user_i2c.h"

extern volatile uint8_t i2c_addr_reg;

/**
 * @brief Load the persisted I2C address or use the default address.
 */
void i2c_address_init(void);

/**
 * @brief Save and apply a pending I2C address change.
 * @note Must be called from the main loop, not from an interrupt.
 */
void i2c_address_update_handler(void);

/**
 * @brief I2C Transaction Complete Callback.
 *        I2C 事务完成回调。
 * @note  Handles both Read preparation (len=1) and Write processing (len>1).
 *        处理读准备 (len=1) 和写处理 (len>1)。
 *
 * @param rx_data Pointer to the received data buffer (first byte is reg addr).
 *                接收数据缓冲区的指针（第一个字节是寄存器地址）。
 * @param len Total length of received data.
 *            接收数据的总长度。
 */
void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len);

/**
 * @brief Called after at least one prepared byte was transmitted.
 */
void Slave_Transmit_Complete_Callback(uint16_t transmitted_len);

/**
 * @brief Clear pending read-completion state after I2C recovery.
 */
void i2c_transaction_state_reset(void);

/**
 * @brief I2C Timeout Handler.
 *        I2C 超时处理程序。
 * @note  Resets the I2C peripheral if a bus lockup/timeout is detected.
 *        如果检测到总线死锁/超时，则复位 I2C 外设。
 */
void i2c_timeout_handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __USER_I2C_CALLBACK_H__ */
