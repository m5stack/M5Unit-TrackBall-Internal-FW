/* Includes ------------------------------------------------------------------*/
#include "user_i2c.h"

// --- Global Variables ---
// --- 全局变量 ---

// Rx Buffer: Stores data received from Master
// 接收缓冲区：存储从主机接收的数据
volatile uint8_t aReceiveBuffer[I2C_RECEIVE_BUFFER_LEN];

// Tx Buffer: Stores data waiting to be sent to Master
// 发送缓冲区：存储等待发送给主机的数据
volatile uint8_t tx_buffer[I2C_RECEIVE_BUFFER_LEN];

volatile uint16_t ubReceiveIndex = 0;  // Current RX index / 当前接收索引
volatile uint8_t i2c_addr = 0;  // Local copy of Slave Address (Left aligned) / 从机地址本地副本 (左对齐)
volatile uint16_t tx_buffer_index = 0;  // Current TX index / 当前发送索引
volatile uint16_t tx_len          = 0;  // Total length to send / 发送总长度
static volatile uint8_t rx_overflow;
static volatile uint8_t tx_transfer_active;

// Set on write address match and cleared when STOP is received or after recovery.
volatile uint32_t i2c_stop_timeout_flag    = 0;
volatile uint32_t i2c_stop_timeout_counter = 0;
volatile uint32_t i2c2_recovery_pending    = 0;

/**
 * @brief Set the internal slave address variable.
 *        设置内部从机地址变量。
 * @param addr 7-bit I2C address. / 7位 I2C 地址。
 */
void set_i2c_slave_address(uint8_t addr)
{
    // STM32 LL Library expects address left-shifted by 1 (8-bit format)
    // STM32 LL 库需要地址左移 1 位（8 位格式）
    i2c_addr = (addr << 1);
}

/**
 * @brief Weak implementation of the callback. Should be defined in user code.
 *        回调的弱实现。应在用户代码中定义。
 */
__weak void Slave_Complete_Callback(uint8_t *rx_data, uint16_t len)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(rx_data);
    UNUSED(len);
}

__weak void Slave_Transmit_Complete_Callback(uint16_t transmitted_len)
{
    UNUSED(transmitted_len);
}

void user_i2c_init(void)
{
    /* USER CODE BEGIN I2C2_Init 0 */

    /* USER CODE END I2C2_Init 0 */

    LL_I2C_InitTypeDef I2C_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    /**I2C2 GPIO Configuration
    PA11 [PA9]   ------> I2C2_SCL
    PA12 [PA10]   ------> I2C2_SDA
    */
    GPIO_InitStruct.Pin        = LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin        = LL_GPIO_PIN_12;
    GPIO_InitStruct.Mode       = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed      = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    GPIO_InitStruct.Pull       = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate  = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);

    /* Keep the IRQ disabled until the peripheral is fully configured. */
    NVIC_DisableIRQ(I2C2_IRQn);
    NVIC_SetPriority(I2C2_IRQn, 0);

    /* USER CODE BEGIN I2C2_Init 1 */

    /* USER CODE END I2C2_Init 1 */

    /** I2C Initialization
     */
    I2C_InitStruct.PeripheralMode  = LL_I2C_MODE_I2C;
    I2C_InitStruct.Timing          = 0x10B17DB5;
    I2C_InitStruct.AnalogFilter    = LL_I2C_ANALOGFILTER_ENABLE;
    I2C_InitStruct.DigitalFilter   = 0;
    I2C_InitStruct.OwnAddress1     = i2c_addr_reg << 1;
    I2C_InitStruct.TypeAcknowledge = LL_I2C_ACK;
    I2C_InitStruct.OwnAddrSize     = LL_I2C_OWNADDRESS1_7BIT;
    LL_I2C_Init(I2C2, &I2C_InitStruct);
    LL_I2C_EnableAutoEndMode(I2C2);
    LL_I2C_SetOwnAddress2(I2C2, 0, LL_I2C_OWNADDRESS2_NOMASK);
    LL_I2C_DisableOwnAddress2(I2C2);
    LL_I2C_DisableGeneralCall(I2C2);
    LL_I2C_EnableClockStretching(I2C2);
    /* USER CODE BEGIN I2C2_Init 2 */

    set_i2c_slave_address(i2c_addr_reg);
    /* USER CODE END I2C2_Init 2 */
}

void i2c2_it_enable(void)
{
    LL_I2C_Enable(I2C2);
    LL_I2C_EnableIT_ADDR(I2C2);  // Address Match / 地址匹配
    LL_I2C_EnableIT_NACK(I2C2);  // NACK received / 接收到 NACK
    LL_I2C_EnableIT_ERR(I2C2);   // Error / 错误
    LL_I2C_EnableIT_STOP(I2C2);  // Stop condition / 停止条件
    NVIC_EnableIRQ(I2C2_IRQn);
}

void i2c2_it_disable(void)
{
    LL_I2C_DisableIT_ADDR(I2C2);
    LL_I2C_DisableIT_NACK(I2C2);
    LL_I2C_DisableIT_ERR(I2C2);
    LL_I2C_DisableIT_STOP(I2C2);
    LL_I2C_DisableIT_RX(I2C2);
    LL_I2C_DisableIT_TX(I2C2);
}

void i2c2_recover(void)
{
    NVIC_DisableIRQ(I2C2_IRQn);
    i2c2_it_disable();
    LL_I2C_Disable(I2C2);
    LL_I2C_DeInit(I2C2);

    ubReceiveIndex           = 0U;
    tx_buffer_index          = 0U;
    tx_len                   = 0U;
    rx_overflow              = 0U;
    tx_transfer_active       = 0U;
    i2c_stop_timeout_flag    = 0U;
    i2c_stop_timeout_counter = 0U;
    i2c2_recovery_pending    = 0U;
    i2c_transaction_state_reset();

    user_i2c_init();
    NVIC_ClearPendingIRQ(I2C2_IRQn);
    i2c2_it_enable();
}

void Error_Callback(void)
{
    i2c2_recovery_pending = 1U;
    i2c2_it_disable();
}

void i2c2_set_send_data(const volatile uint8_t *tx_ptr, uint16_t len)
{
    uint16_t index;

    if (len > I2C_RECEIVE_BUFFER_LEN) {
        len = I2C_RECEIVE_BUFFER_LEN;
    }

    if (len == 0U || tx_ptr == NULL) {
        tx_buffer_index = 0U;
        tx_len          = 0U;
        return;
    }
    for (index = 0U; index < len; index++) {
        tx_buffer[index] = tx_ptr[index];
    }
    tx_buffer_index = 0;
    tx_len          = len;
}

void Slave_Reception_Callback(void)
{
    uint8_t data = LL_I2C_ReceiveData8(I2C2);

    if (ubReceiveIndex < I2C_RECEIVE_BUFFER_LEN) {
        aReceiveBuffer[ubReceiveIndex++] = data;
    } else {
        rx_overflow = 1U;
    }
}

void Slave_Ready_To_Transmit_Callback(void)
{
    uint8_t data = I2C_INVALID_DATA;

    if (tx_buffer_index < tx_len) {
        data = tx_buffer[tx_buffer_index++];
    }

    LL_I2C_TransmitData8(I2C2, data);
}

void I2C2_IRQHandler(void)
{
    /* USER CODE BEGIN I2C2_IRQn 0 */
    if (LL_I2C_IsActiveFlag_BERR(I2C2) || LL_I2C_IsActiveFlag_ARLO(I2C2) || LL_I2C_IsActiveFlag_OVR(I2C2)) {
        if (LL_I2C_IsActiveFlag_BERR(I2C2)) {
            LL_I2C_ClearFlag_BERR(I2C2);
        }
        if (LL_I2C_IsActiveFlag_ARLO(I2C2)) {
            LL_I2C_ClearFlag_ARLO(I2C2);
        }
        if (LL_I2C_IsActiveFlag_OVR(I2C2)) {
            LL_I2C_ClearFlag_OVR(I2C2);
        }
        Error_Callback();
        return;
    }

    /* Check ADDR flag value in ISR register */
    if (LL_I2C_IsActiveFlag_ADDR(I2C2)) {
        /* Verify the Address Match with the OWN Slave address */
        if (LL_I2C_GetAddressMatchCode(I2C2) == i2c_addr) {
            if ((tx_transfer_active != 0U) && (tx_buffer_index != 0U)) {
                Slave_Transmit_Complete_Callback(tx_buffer_index);
            }
            tx_transfer_active = 0U;
            tx_buffer_index    = 0U;
            if (ubReceiveIndex != 0U || rx_overflow != 0U) {
                i2c2_it_disable();
                if (rx_overflow == 0U) {
                    Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
                } else {
                    i2c2_set_send_data(NULL, 0U);
                }
                ubReceiveIndex = 0U;
                rx_overflow    = 0U;
                i2c2_it_enable();
            }
            /* Verify the transfer direction, a write direction, Slave enters receiver mode */
            if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_WRITE) {
                /* Clear ADDR flag value in ISR register */
                LL_I2C_ClearFlag_ADDR(I2C2);

                /* Enable Receive Interrupt */
                LL_I2C_EnableIT_RX(I2C2);
                i2c_stop_timeout_counter = 0U;
                i2c_stop_timeout_flag    = 1U;
            }
            /* Verify the transfer direction, a read direction, Slave enters transmitter mode */
            else if (LL_I2C_GetTransferDirection(I2C2) == LL_I2C_DIRECTION_READ) {
                /* Clear ADDR flag value in ISR register */
                LL_I2C_ClearFlag_ADDR(I2C2);

                tx_transfer_active = 1U;
                /* Enable Transmit Interrupt */
                LL_I2C_EnableIT_TX(I2C2);
            } else {
                /* Clear ADDR flag value in ISR register */
                LL_I2C_ClearFlag_ADDR(I2C2);

                /* Call Error function */
                Error_Callback();
            }
        } else {
            /* Clear ADDR flag value in ISR register */
            LL_I2C_ClearFlag_ADDR(I2C2);

            /* Call Error function */
            Error_Callback();
        }
    }
    /* Check NACK flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_NACK(I2C2)) {
        /* End of Transfer */
        LL_I2C_ClearFlag_NACK(I2C2);
    }
    /* Check TXIS flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_TXIS(I2C2)) {
        /* Call function Slave Ready to Transmit Callback */
        Slave_Ready_To_Transmit_Callback();
    }
    /* Check RXNE flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_RXNE(I2C2)) {
        /* Call function Slave Reception Callback */
        Slave_Reception_Callback();
    }
    /* Check STOP flag value in ISR register */
    else if (LL_I2C_IsActiveFlag_STOP(I2C2)) {
        uint16_t transmitted_len = tx_buffer_index;

        /* End of Transfer */
        LL_I2C_ClearFlag_STOP(I2C2);

        /* Check TXE flag value in ISR register */
        if (!LL_I2C_IsActiveFlag_TXE(I2C2)) {
            /* Flush the TXDR register */
            LL_I2C_ClearFlag_TXE(I2C2);
        }

        i2c2_it_disable();
        if (transmitted_len != 0U) {
            Slave_Transmit_Complete_Callback(transmitted_len);
        }
        tx_transfer_active = 0U;
        /* Call function Slave Complete Callback */
        if (rx_overflow == 0U) {
            Slave_Complete_Callback((uint8_t *)aReceiveBuffer, ubReceiveIndex);
        } else {
            i2c2_set_send_data(NULL, 0U);
        }
        ubReceiveIndex           = 0U;
        rx_overflow              = 0U;
        i2c_stop_timeout_flag    = 0U;
        i2c_stop_timeout_counter = 0U;
        i2c2_it_enable();
    }
    /* Check TXE flag value in ISR register */
    else if (!LL_I2C_IsActiveFlag_TXE(I2C2)) {
        /* Do nothing */
        /* This Flag will be set by hardware when the TXDR register is empty */
        /* If needed, use LL_I2C_ClearFlag_TXE() interface to flush the TXDR register */
    } else {
        /* Call Error function */
        Error_Callback();
    }
    /* USER CODE END I2C2_IRQn 0 */

    /* USER CODE BEGIN I2C2_IRQn 1 */

    /* USER CODE END I2C2_IRQn 1 */
}
