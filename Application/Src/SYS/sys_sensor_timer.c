/*
 * sys_sensor_timer.c
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"

#include "sys_sensor_timer.h"
#include "sys_temperature.h"
#include "sys_power.h"

#include "error_manager.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define SENSOR_TIMER_SOURCE             htim16

#define SENSOR_TIMER_TICK_RATE_HZ       1000
// Trigger a temeprature sensor reading every 200ms
#define TEMPERATURE_SENSOR_PERIOD_MS    200
// Power reading every 1ms
#define INA_PERIOD_MS                   1

extern TIM_HandleTypeDef SENSOR_TIMER_SOURCE;

#define TICKS_FOR_TEMPERATURE ((TEMPERATURE_SENSOR_PERIOD_MS * \
                                SENSOR_TIMER_TICK_RATE_HZ) / \
                                1000)

#define TICKS_FOR_INA219 ((INA_PERIOD_MS * \
                           SENSOR_TIMER_TICK_RATE_HZ) / \
                           1000)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint32_t sensor_ticks = 0;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void SensorTimer_Init()
{
  RETURN_IF_ERROR_PRESENT();
  if (HAL_TIM_Base_Start_IT(&SENSOR_TIMER_SOURCE) != HAL_OK) 
    REGISTER_ERROR(ERROR_TIMER_INITIALIZATION);

  sensor_ticks = 0;
}

void SensorTimer_Tick()
{
  sensor_ticks++;

  if ((sensor_ticks % TICKS_FOR_TEMPERATURE) == 0) {
    Temperature_TriggerTjConversion();
  }
  if ((sensor_ticks % TICKS_FOR_INA219) == 0) {
    INA_StartRead();
  }
}

/* Private function definitions ----------------------------------------------*/
