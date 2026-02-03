/*
 * tr_switch.c
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
#include "internal/tr_switch.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static TrState_t current_state = TR_NONE;

/* Private function prototypes -----------------------------------------------*/

static void switchInput(void);
static void switchOutput(void);
static void switchOff(void);

static void driveTrSel(GPIO_PinState state);
static void driveTrEn(GPIO_PinState state);

/* Exported function definitions ---------------------------------------------*/

void TR_Change(TrState_t new_state)
{
  switch (new_state) {
    case TR_INPUT_MODE:
      switchInput();
      break;
    case TR_OUTPUT_MODE:
      switchOutput();
      break;
    case TR_NONE:
      switchOff();
      break;
    default:
  }
}

TrState_t TR_CurrentState(void)
{
  return current_state;
}

/* Private function definitions ----------------------------------------------*/

void switchInput(void)
{
  driveTrSel(GPIO_PIN_RESET);
  driveTrEn(GPIO_PIN_SET);

  current_state = TR_INPUT_MODE;
}

void switchOutput(void)
{
  driveTrSel(GPIO_PIN_SET);
  driveTrEn(GPIO_PIN_SET);

  current_state = TR_OUTPUT_MODE;
}

void switchOff(void)
{
  driveTrEn(GPIO_PIN_RESET);

  current_state = TR_NONE;
}

void driveTrSel(GPIO_PinState state)
{
  HAL_GPIO_WritePin(TR_SEL_GPIO_Port, TR_SEL_Pin, state);
}

void driveTrEn(GPIO_PinState state)
{
  HAL_GPIO_WritePin(TR_EN_GPIO_Port, TR_EN_Pin, state);
}
