/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    iwdg.c
  * @brief   This file provides code for the configuration
  *          of the IWDG instance.
  ******************************************************************************
  */
/* USER CODE END Header */
#include "iwdg.h"

void MX_IWDG_Init(void)
{
  LL_IWDG_Enable(IWDG);
  LL_IWDG_EnableWriteAccess(IWDG);
  LL_IWDG_SetPrescaler(IWDG, USER_IWDG_PRESCALER);
  LL_IWDG_SetReloadCounter(IWDG, USER_IWDG_RELOAD_VALUE);
  while (LL_IWDG_IsReady(IWDG) != 1U)
  {
  }

  LL_IWDG_ReloadCounter(IWDG);
}
