/*
 * Copyright (c) 2022 MixoSense Technology Ltd <contact@mixosense.com>.
 *
 * All rights are reserved.
 * Proprietary and confidential.
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * Any use is subject to an appropriate license granted by MixoSense Technology
 * Ltd.
 *
 */

/*-----------------------------------------------------------------------------
 * HEADER FILES
 *---------------------------------------------------------------------------*/
#include "mixo_ots_access.h"

/**
 * inlcude from STM32 HAL
 */
#include "mixo_hal.h"  // support printf、
#include "mixo_gpio_i2c.h"
static uint8_t slave_addr = 0x74;
static uint8_t i2c_error_count;

static int mixo_ots_record_result(int result)
{
    if (result != 0 && i2c_error_count != 0xFFU) {
        i2c_error_count++;
    }
    return result;
}

void mixo_ots_set_slave_id(uint8_t slave_id)
{
    slave_addr = slave_id;
}
int mixo_ots_reg_read(uint8_t regAddr, uint8_t* regData)
{
    int result;

    if (regData == NULL) {
        return mixo_ots_record_result(1);
    }
    *regData = 0x00;  // 清空数据
    result   = mixo_gpio_i2c_write_read(slave_addr, &regAddr, 1, regData, 1);
    return mixo_ots_record_result(result);
}

int mixo_ots_reg_consecutive_read(uint8_t regStartAddr, int len, uint8_t* regData)
{
    int rc = 0;
    if (regData == NULL || len <= 0) {
        return mixo_ots_record_result(1);
    }
    rc = mixo_gpio_i2c_write_read(slave_addr, &regStartAddr, 1, regData, len);
    return mixo_ots_record_result(rc);
}

int mixo_ots_reg_write(uint8_t regAddr, uint8_t regData)
{
    uint8_t tmp[2] = {regAddr, regData};
    int result     = mixo_gpio_i2c_write_read(slave_addr, tmp, 2, NULL, 0);

    return mixo_ots_record_result(result);
}

uint8_t mixo_ots_get_i2c_error_count(void)
{
    return i2c_error_count;
}

void mixo_ots_clear_i2c_error_count(void)
{
    i2c_error_count = 0U;
}
