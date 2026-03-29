/*
 * pwr_domains.c
 *
 *  Created on: Dec 28, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "stm32h723xx.h"
#include "main.h"
#include "pwr_domains.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  PowerDomainState_t state;
  bool transitioning_on;
  uint64_t transition_finished_timestamp;
} PowerDomainInfo_t;

/* Private define ------------------------------------------------------------*/

// TODO: Look at datasheets to calculate this
#define STABILIZATION_DELAY_3V3A            5
#define STABILIZATION_DELAY_NEGATIVE_3V3    5
#define STABILIZATION_DELAY_30V             200
#define STABILIZATION_DELAY_WS_5V           5

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

PowerDomainInfo_t info_3V3A = {
  .state = PWR_OFF,
  .transition_finished_timestamp = 0
};

PowerDomainInfo_t info_negative_3V3 = {
  .state = PWR_OFF,
  .transition_finished_timestamp = 0
};

PowerDomainInfo_t info_30V = {
  .state = PWR_OFF,
  .transition_finished_timestamp = 0
};

PowerDomainInfo_t info_Ws5V = {
  .state = PWR_OFF,
  .transition_finished_timestamp = 0
};

/* Private function prototypes -----------------------------------------------*/

static void switch3V3A(bool on);
static void switchNegative3V3(bool on);
static void switch30V(bool on);
static void switchWs5V(bool on);

static void updateStates();
static void updateState(PowerDomainInfo_t* domain_info);

/* Exported function definitions ---------------------------------------------*/

void PWR_Analog(bool on)
{
  switch3V3A(on);
  switchNegative3V3(on);
}

void PWR_30V(bool on)
{
  switch30V(on);
}

void PWR_Ws5V(bool on)
{
  switchWs5V(on);
}

PowerDomainState_t PWR_StateAnalog(void)
{
  updateStates();
  PowerDomainState_t state1 = info_3V3A.state;
  PowerDomainState_t state2 = info_negative_3V3.state;
  if (state1 == PWR_OFF || state2 == PWR_OFF) {
    return PWR_OFF;
  }
  else if (state1 == PWR_READY && state2 == PWR_READY) {
    return PWR_READY;
  }
  else { // If we are here, neither is off nor both on so either one ready one transition or both transitioning
    return PWR_TRANSITIONING;
  }
}

PowerDomainState_t PWR_State30V(void)
{
  updateStates();
  return info_30V.state;
}

PowerDomainState_t PWR_StateWs5V(void)
{
  updateStates();
  return info_Ws5V.state;
}

/* Private function definitions ----------------------------------------------*/

void switch3V3A(bool on)
{
  if (((on == true)  && (info_3V3A.state == PWR_READY)) ||
      ((on == false) && (info_3V3A.state == PWR_OFF))) {
    return;
  }
  HAL_GPIO_WritePin(EN_3V3A_GPIO_Port, EN_3V3A_Pin, on);
  info_3V3A.state = PWR_TRANSITIONING;
  info_3V3A.transitioning_on = on;
  info_3V3A.transition_finished_timestamp = HAL_AbsoluteTimestamp() + STABILIZATION_DELAY_3V3A;
}

void switchNegative3V3(bool on)
{
  if (((on == true)  && (info_negative_3V3.state == PWR_READY)) ||
      ((on == false) && (info_negative_3V3.state == PWR_OFF))) {
    return;
  }
  HAL_GPIO_WritePin(EN__5V_GPIO_Port, EN__5V_Pin, on);
  info_negative_3V3.state = PWR_TRANSITIONING;
  info_negative_3V3.transitioning_on = on;
  info_negative_3V3.transition_finished_timestamp = HAL_AbsoluteTimestamp() + STABILIZATION_DELAY_NEGATIVE_3V3;
}

void switch30V(bool on)
{
  if (((on == true)  && (info_30V.state == PWR_READY)) ||
      ((on == false) && (info_30V.state == PWR_OFF))) {
    return;
  }
  HAL_GPIO_WritePin(EN_30V_GPIO_Port, EN_30V_Pin, on);
  info_30V.state = PWR_TRANSITIONING;
  info_30V.transitioning_on = on;
  info_30V.transition_finished_timestamp = HAL_AbsoluteTimestamp() + STABILIZATION_DELAY_30V;
}

void switchWs5V(bool on)
{
  if (((on == true)  && (info_Ws5V.state == PWR_READY)) ||
      ((on == false) && (info_Ws5V.state == PWR_OFF))) {
    return;
  }
  HAL_GPIO_WritePin(WS_EN_GPIO_Port, WS_EN_Pin, on);
  info_Ws5V.state = PWR_TRANSITIONING;
  info_Ws5V.transitioning_on = on;
  info_Ws5V.transition_finished_timestamp = HAL_AbsoluteTimestamp() + STABILIZATION_DELAY_WS_5V;
}

void updateStates()
{
  updateState(&info_3V3A);
  updateState(&info_negative_3V3);
  updateState(&info_30V);
  if (HAL_GPIO_ReadPin(PGOOD_30V_GPIO_Port, PGOOD_30V_Pin) == GPIO_PIN_RESET) {
    info_30V.state = PWR_ERROR;
  }
  updateState(&info_Ws5V);
}

void updateState(PowerDomainInfo_t* domain_info)
{
  uint64_t current_timestamp = HAL_AbsoluteTimestamp();
  bool is_transitioning = domain_info->state == PWR_TRANSITIONING;
  bool transition_finished = domain_info->transition_finished_timestamp <= current_timestamp;
  if (is_transitioning && transition_finished) {
    if (domain_info->transitioning_on == true) {
      domain_info->state = PWR_READY;
    }
    else {
      domain_info->state = PWR_OFF;
    }
  } 
}
