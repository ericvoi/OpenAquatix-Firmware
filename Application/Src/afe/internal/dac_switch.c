/*
 * dac_switch.c
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
#include "main.h"
#include "internal/dac_switch.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static DacDirection_t dac_direction = DAC_DIRECTION_FEEDBACK;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void DACSwitch_Change(DacDirection_t direction)
{
  GPIO_PinState pin_state = (direction == DAC_DIRECTION_TRANSDUCER) ? GPIO_PIN_RESET : GPIO_PIN_SET;
  HAL_GPIO_WritePin(DAC_SEL_GPIO_Port, DAC_SEL_Pin, pin_state);
  dac_direction = direction;
}

DacDirection_t DACSwitch_Current(void)
{
  return dac_direction;
}

/* Private function definitions ----------------------------------------------*/
