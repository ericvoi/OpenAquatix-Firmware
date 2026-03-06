/*
 * bkpsram_layout.c
 *
 *  Created on: Feb 23, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "bkpsram_layout.h"
#include "error_reset.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Exported variables --------------------------------------------------------*/

volatile BkpSramData_t bkpsram __attribute__((section(".bkpsram"), used));

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void Bkpsram_Init(void)
{
  RCC->RSR |= RCC_RSR_RMVF;

  if (bkpsram.error_reset_flag == ERROR_RESET_MAGIC_NUMBER) {
    bkpsram.error_reset_flag = 0;
    bkpsram.reset_count++;
    ErrorReset_NotifyErrorReset();
  } 
  else {
    memset((void*)&bkpsram, 0, sizeof(bkpsram));
  }
}

/* Private function definitions ----------------------------------------------*/
