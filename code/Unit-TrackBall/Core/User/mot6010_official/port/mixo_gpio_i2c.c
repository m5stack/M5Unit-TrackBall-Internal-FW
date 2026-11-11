#include "mixo_gpio_i2c.h"

#include "i2c.h"
#include "stm32g0xx_hal.h"

#define MIXO_I2C_TIMEOUT_MS (20U)

uint8_t mixo_gpio_i2c_write_read(uint8_t slave_id, uint8_t *write_buffer, size_t write_size, uint8_t *read_buffer,
                                 size_t read_size)
{
    HAL_StatusTypeDef status;
    uint16_t dev_addr = (uint16_t)(slave_id << 1);

    if (write_buffer == NULL || write_size == 0U) {
        return 1;
    }

    if (read_buffer != NULL && read_size > 0U) {
        if (write_size != 1U || read_size > UINT16_MAX) {
            return 1;
        }

        status = HAL_I2C_Mem_Read(&hi2c1, dev_addr, write_buffer[0], I2C_MEMADD_SIZE_8BIT, read_buffer,
                                  (uint16_t)read_size, MIXO_I2C_TIMEOUT_MS);
        return (status == HAL_OK) ? 0 : 1;
    }

    if (write_size < 2U || (write_size - 1U) > UINT16_MAX) {
        return 1;
    }

    status = HAL_I2C_Mem_Write(&hi2c1, dev_addr, write_buffer[0], I2C_MEMADD_SIZE_8BIT, &write_buffer[1],
                               (uint16_t)(write_size - 1U), MIXO_I2C_TIMEOUT_MS);
    return (status == HAL_OK) ? 0 : 1;
}

void mixo_i2c_set_timing_register(uint32_t timing_register)
{
    (void)timing_register;
}

void mixo_i2c_set_check_count(int count)
{
    (void)count;
}
