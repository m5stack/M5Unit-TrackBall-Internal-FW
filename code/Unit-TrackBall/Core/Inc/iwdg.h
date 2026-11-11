/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    iwdg.h
  * @brief   This file contains all the function prototypes for
  *          the iwdg.c file.
  ******************************************************************************
  */
/* USER CODE END Header */
#ifndef __IWDG_H__
#define __IWDG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

// Nominal timeout: (2999 + 1) * 32 / 32000 Hz = 3 seconds.
#define USER_IWDG_TIMEOUT_MS   (3000U)
#define USER_IWDG_PRESCALER    LL_IWDG_PRESCALER_32
#define USER_IWDG_RELOAD_VALUE (2999U)

void MX_IWDG_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __IWDG_H__ */
