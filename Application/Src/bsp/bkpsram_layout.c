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
  // Check if reset source is POR. If so, reset junk values
  if (RCC->RSR & RCC_RSR_PORRSTF) {
    memset((void*)&bkpsram, 0, sizeof(bkpsram));

    RCC->RSR |= RCC_RSR_RMVF;
  }
  else {
    bkpsram.reset_count++;
  }
}

/* Private function definitions ----------------------------------------------*/
