/*
 * Minimal HAL facade required by the official MixoSense OTS driver.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

bool mixo_hal_get_mot_level(void);
