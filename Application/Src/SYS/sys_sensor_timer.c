/*
 * sys_sensor_timer.c
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"

#include "sys_sensor_timer.h"
#include "sys_temperature.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define TICKS_FOR_TEMPERATURE ((TEMPERATURE_SENSOR_PERIOD_MS * \
                                SENSOR_TIMER_TICK_RATE_HZ) / \
                                1000)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint32_t sensor_ticks = 0;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool SensorTimer_Init()
{
  if (HAL_TIM_Base_Start_IT(&SENSOR_TIMER_SOURCE) != HAL_OK) {
    return false;
  }

  return true;
}

void SensorTimer_Tick()
{
  sensor_ticks++;

  if ((sensor_ticks % TICKS_FOR_TEMPERATURE) == 0) {
    Temperature_TriggerConversion();
  }
}

/* Private function definitions ----------------------------------------------*/
