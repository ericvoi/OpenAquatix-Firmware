/*
 * power_leds.c
 *
 *  Created on: Dec 28, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "main.h"
#include "power_leds.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  GPIO_TypeDef* port;
  uint16_t pin;
} LedPin_t;

/* Private define ------------------------------------------------------------*/

#define NUM_PWR_LEDS      4

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

LedPin_t pin_locations[NUM_PWR_LEDS] = {
  // 0 -> 3V3 (digital)
  {
    .port = EN_3V3_LED_GPIO_Port,
    .pin = EN_3V3_LED_Pin
  },

  // 1 -> 3V3A (analog)
  {
    .port = EN_3V3A_LED_GPIO_Port,
    .pin = EN_3V3A_LED_Pin
  },

  // 2 -> 30V
  {
    .port = EN_30V_LED_GPIO_Port,
    .pin = EN_30V_LED_Pin
  },

  // 3 -> BATT
  {
    .port = EN_BATT_LED_GPIO_Port,
    .pin = EN_BATT_LED_Pin
  }
};

uint8_t current_states = 0U;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void PWRLED_Update(uint8_t states)
{
  for (uint8_t i = 0; i < NUM_PWR_LEDS; i++) {
    uint8_t new_state = (states >> i) & 1;
    HAL_GPIO_WritePin(pin_locations[i].port, pin_locations[i].pin, new_state);
  }
  current_states = states;
}

uint8_t PWRLED_CurrentState(void)
{
  return current_states;
}

/* Private function definitions ----------------------------------------------*/
