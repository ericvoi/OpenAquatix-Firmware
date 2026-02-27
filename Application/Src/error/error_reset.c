/*
 * error_reset.c
 *
 *  Created on: Feb 25, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "bkpsram_layout.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void ErrorReset_WarmReset()
{
  bkpsram.log_reset_flag = LOG_RESET_MAGIC_NUMBER;
  __DSB();
  HAL_NVIC_SystemReset();
}

/* Private function definitions ----------------------------------------------*/
