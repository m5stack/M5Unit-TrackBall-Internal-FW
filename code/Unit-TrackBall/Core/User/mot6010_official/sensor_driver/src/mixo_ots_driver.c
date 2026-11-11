#include "mixo_ots_driver.h"

#include "mixo_ots_access.h"

#include "mixo_hal_timer.h"  // support mixo_hal_delay_ms、

// macro definitions about reigster data
#define REG_PROTECT_ID_HIGH 0x58
#define REG_PROTECT_ID_LOW  0x5B

/**
 * @brief 接收1byte数据
 * @param[in] original_dx   低8bit的dx数据
 * @param[in] original_dy   低8bit的dy数据
 * @param[in] high_bit      高4bit的dx/dy数据
 * @param[out] dx           合成后的dx数据
 * @param[out] dx           合成后的dy数据
 * */
static void data_synthesis(uint8_t original_dx, uint8_t original_dy, uint8_t high_bit, int16_t* dx, int16_t* dy)
{
    // move high 4 bits to bit 11 ~ bit 8
    int16_t dx_high = (high_bit << 4) & 0x0F00;
    int16_t dy_high = (high_bit << 8) & 0x0F00;

    // sign extension
    if (dx_high & 0x0800) {
        dx_high |= 0xF000;
    }
    if (dy_high & 0x0800) {
        dy_high |= 0xF000;
    }

    // move low 8 bits to bit 7 ~ bit 0
    *dx = dx_high | (int16_t)original_dx;
    *dy = dy_high | (int16_t)original_dy;
}

int mixo_ots_get_mot_status(uint8_t* mot_status)
{
    int ret                = 0;
    uint8_t tmp_mot_status = 0x00;
    ret                    = mixo_ots_reg_read(0x02, &tmp_mot_status);  // get MOTION STATUS
    if (ret != 0) goto Exit;
    *mot_status = tmp_mot_status;

Exit:
    return ret;
}

int mixo_ots_get_raw_data(raw_data_t* raw_data)
{
    int ret                 = 0;
    uint8_t tmp_dx          = 0x00;
    uint8_t tmp_dy          = 0x00;
    uint8_t tmp_dx_dy_hight = 0x00;

    if (raw_data == NULL) {
        ret = -1;
        goto Exit;
    }

    ret = mixo_ots_reg_read(0x12, &tmp_dx_dy_hight);  // get DELTA XY HIGH
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(0x03, &tmp_dx);  // get DELTA X
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(0x04, &tmp_dy);  // get DELTA Y
    if (ret != 0) goto Exit;

    data_synthesis(tmp_dx, tmp_dy, tmp_dx_dy_hight, &(raw_data->dx), &(raw_data->dy));

Exit:
    return ret;
}

/* 强制清除MOT的中断信号 */
int mixo_ots_forced_clear_int(void)
{
    uint8_t tmp = 0x00;

    if (mixo_ots_reg_read(0x02, &tmp) != 0) {
        return -1;
    }
    if (mixo_ots_reg_read(0x02, &tmp) != 0) {
        return -1;
    }

    return 0;
}

/* 进入/退出Suspend模式 */
void mixo_ots_enter_suspend_mode(uint8_t true_or_false)
{
    uint8_t tmp = 0;
    // 将数据从寄存器中读出后进行位操作，再重新写回
    if (true_or_false) {
        // bit 3 写 1， 进入Suspend
        mixo_ots_reg_read(0x06, &tmp);
        mixo_ots_reg_write(0x06, (tmp | 0x08));
    } else {
        // bit 3 写 0， 退出Suspend
        mixo_ots_reg_read(0x06, &tmp);
        mixo_ots_reg_write(0x06, (tmp & 0xF7));
    }
}

int callisto_2x_init(void)
{
    int ret;
    uint8_t id_high       = 0x00;
    uint8_t id_low        = 0x00;
    uint8_t tmp_read      = 0x00;
    uint8_t sensor_option = (uint8_t)(MIXO_OTS_CONFIG2_12BIT | MIXO_OTS_CONFIG2_Y_INVERT);

    ret = mixo_ots_reg_write(0x09, 0x5A);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x3C, 0xAC);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x09, 0xA5);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x6D, 0x39);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(0x00, &id_high);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(0x01, &id_low);
    if (ret != 0) goto Exit;

    if ((id_high != REG_PROTECT_ID_HIGH) || (id_low != REG_PROTECT_ID_LOW)) {
        ret = -1;
        goto Exit;
    }

    ret = mixo_ots_reg_write(0x19, sensor_option);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(MIXO_OTS_REG_OP_MODE, MIXO_OTS_OP_MODE_ACTIVE);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(MIXO_OTS_REG_OP_MODE, &tmp_read);
    if ((ret != 0) || (tmp_read != MIXO_OTS_OP_MODE_ACTIVE)) {
        ret = -1;
        goto Exit;
    }
    ret = mixo_ots_reg_write(0x3E, 0x73);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x48, 0x0F);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x1F, 0xFE);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x73, 0x08);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x68, 0x67);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x7D, 0x60);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x60, 0x00);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x0D, 0x20);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_write(0x0E, 0x20);
    if (ret != 0) goto Exit;
    ret = mixo_ots_reg_read(MIXO_OTS_REG_CONFIG2, &tmp_read);
    if (ret != 0) goto Exit;
    if (tmp_read != sensor_option) {
        ret = -1;
    }

Exit:
    return ret;
}

int mixo_ots_apply_axis_config(uint8_t user_config)
{
    int ret;
    int rollback_ret = 0;
    uint8_t config2  = MIXO_OTS_CONFIG2_12BIT;
    uint8_t old_config2;
    uint8_t readback;

    if ((user_config & (uint8_t)~0x07U) != 0U) {
        return MIXO_OTS_CONFIG_FAILED;
    }
    if ((user_config & 0x01U) != 0U) {
        config2 |= MIXO_OTS_CONFIG2_X_INVERT;
    }
    /* The device's Y direction is inverted relative to the user-level default. */
    if ((user_config & 0x02U) == 0U) {
        config2 |= MIXO_OTS_CONFIG2_Y_INVERT;
    }
    if ((user_config & 0x04U) != 0U) {
        config2 |= MIXO_OTS_CONFIG2_XY_SWAP;
    }

    if (mixo_ots_reg_read(MIXO_OTS_REG_CONFIG2, &old_config2) != 0) {
        return MIXO_OTS_CONFIG_STATE_UNKNOWN;
    }

    ret = mixo_ots_reg_write(MIXO_OTS_REG_WRITE_PROTECT, MIXO_OTS_WRITE_PROTECT_OFF);
    if (ret == 0) {
        ret = mixo_ots_reg_write(MIXO_OTS_REG_CONFIG2, config2);
    }
    if (ret == 0) {
        ret = mixo_ots_reg_read(MIXO_OTS_REG_CONFIG2, &readback);
    }
    if ((ret == 0) && (readback != config2)) {
        ret = MIXO_OTS_CONFIG_FAILED;
    }
    if ((ret == 0) && (mixo_ots_reg_write(MIXO_OTS_REG_WRITE_PROTECT, MIXO_OTS_WRITE_PROTECT_ON) == 0)) {
        return MIXO_OTS_CONFIG_OK;
    }

    if (mixo_ots_reg_write(MIXO_OTS_REG_WRITE_PROTECT, MIXO_OTS_WRITE_PROTECT_OFF) != 0) {
        rollback_ret = 1;
    }
    if (mixo_ots_reg_write(MIXO_OTS_REG_CONFIG2, old_config2) != 0) {
        rollback_ret = 1;
    }
    if ((mixo_ots_reg_read(MIXO_OTS_REG_CONFIG2, &readback) != 0) || (readback != old_config2)) {
        rollback_ret = 1;
    }
    if (mixo_ots_reg_write(MIXO_OTS_REG_WRITE_PROTECT, MIXO_OTS_WRITE_PROTECT_ON) != 0) {
        rollback_ret = 1;
    }

    return (rollback_ret == 0) ? MIXO_OTS_CONFIG_FAILED : MIXO_OTS_CONFIG_STATE_UNKNOWN;
}

int mixo_ots_init(void)
{
    int rc;
    uint8_t HardWareId = 0;

    rc = mixo_ots_reg_read(0x1B, &HardWareId);
    if (rc != 0) {
        return rc;
    }

    switch (HardWareId) {
        case 0xA1:
            rc = callisto_2x_init();
            break;
        case 0xB1:
            rc = callisto_2x_init();
            break;
        case 0xC1:
            rc = callisto_2x_init();
            break;
        default:
            rc = -1;
            break;
    }

    return rc;
}
