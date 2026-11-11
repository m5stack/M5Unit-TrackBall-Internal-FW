/*
 * Copyright (c) 2019 MixoSense Technology Ltd <contact@mixosense.com>.
 *
 * All rights are reserved.
 * Proprietary and confidential.
 * Unauthorized copying of this file, via any medium is strictly prohibited.
 * Any use is subject to an appropriate license granted by MixoSense Technology
 * Ltd.
 *
 */

#ifndef _MXS_DRIVER_H_
#define _MXS_DRIVER_H_

#include <stdint.h>

#define MIXO_OTS_REG_OP_MODE       (0x05U)
#define MIXO_OTS_REG_CONFIG2       (0x19U)
#define MIXO_OTS_OP_MODE_ACTIVE    (0xA8U)
#define MIXO_OTS_CONFIG2_12BIT     (0x04U)
#define MIXO_OTS_CONFIG2_X_INVERT  (0x08U)
#define MIXO_OTS_CONFIG2_Y_INVERT  (0x10U)
#define MIXO_OTS_CONFIG2_XY_SWAP   (0x20U)
#define MIXO_OTS_REG_WRITE_PROTECT (0x09U)
#define MIXO_OTS_WRITE_PROTECT_OFF (0xA5U)
#define MIXO_OTS_WRITE_PROTECT_ON  (0x00U)

#define MIXO_OTS_CONFIG_OK            (0)
#define MIXO_OTS_CONFIG_FAILED        (1)
#define MIXO_OTS_CONFIG_STATE_UNKNOWN (2)

typedef struct {
    int16_t dx;
    int16_t dy;
} raw_data_t;

int mixo_ots_init(void);
int mixo_ots_apply_axis_config(uint8_t user_config);
int mixo_ots_get_mot_status(uint8_t* mot_status);
int mixo_ots_get_raw_data(raw_data_t* raw_data);
int mixo_ots_forced_clear_int(void);
void mixo_ots_enter_suspend_mode(uint8_t true_or_false);

#endif  //_MXS_DRIVER_H_
