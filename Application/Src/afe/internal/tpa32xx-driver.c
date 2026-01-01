/*
 * tpa32xx-driver.c
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "internal/tpa32xx-driver.h"
#include "pwr_domains.h"
#include "main.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define RESET_GPIO_PORT         TPA_RESET_GPIO_Port
#define RESET_GPIO_PIN          TPA_RESET_Pin
#define FILTER_GPIO_PORT        PAMP_FILTER_EN_GPIO_Port
#define FILTER_GPIO_PIN         PAMP_FILTER_EN_Pin

#define TIMEOUT_DELAY_MS        1000

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static TpaState_t current_state = TPA_OFF;
static bool filter_active = false;

/* Private function prototypes -----------------------------------------------*/

static bool switchReset(bool on);
static bool switchFilter(bool on);

/* Exported function definitions ---------------------------------------------*/

// TODO: Handle errors on FAULT and CLIP_OTW
bool TPA_Enable(void)
{
  PWR_Analog(true);
  PWR_30V(true);
  uint64_t start_timestamp = HAL_AbsoluteTimestamp();
  while ((PWR_StateAnalog() == PWR_TRANSITIONING) || (PWR_State30V() == PWR_TRANSITIONING)) {
    if (HAL_AbsoluteTimestamp() - start_timestamp > TIMEOUT_DELAY_MS) return false;
    osDelay(1);
  }
  if ((PWR_StateAnalog() != PWR_READY) || (PWR_State30V() != PWR_READY)) return false;

  if (TPA_Unmute() == false) return false;

  current_state = TPA_ACTIVE;
  return true;
}

bool TPA_Mute(void)
{
  if (switchReset(false) == false) return false;
  if (switchFilter(true) == false) return false;
  return true;
}

bool TPA_Unmute(void)
{
  if (switchReset(true) == false) return false;
  if (switchFilter(false) == false) return false;
  return true;
}

bool TPA_Shutdown(void)
{
  PWR_30V(false);

  if (TPA_Mute() == false) return false;

  current_state = TPA_OFF;
  return true;
}

TpaState_t TPA_State(void)
{
  return current_state;
}

bool TPA_Ready(void)
{
  if (current_state != TPA_ACTIVE) return false;
  return !filter_active;
}

/* Private function definitions ----------------------------------------------*/

bool switchReset(bool on)
{
  // The RESET pin is hardware protected so no need to check if TPA powered on
  HAL_GPIO_WritePin(RESET_GPIO_PORT, RESET_GPIO_PIN, on);
  if (on == true) {
    current_state = TPA_ACTIVE;
  }
  else if (PWR_State30V() == PWR_READY) { // Powered on but reset asserted
    current_state = TPA_MUTED;
  }
  else { // Powered off and reset 
    current_state = TPA_OFF;
  }
  return true;
}

bool switchFilter(bool on)
{
  // Filter EN is protected by hardware so the uC cannot damage the filter
  HAL_GPIO_WritePin(FILTER_GPIO_PORT, FILTER_GPIO_PIN, on);
  filter_active = on;
  return true;
}
