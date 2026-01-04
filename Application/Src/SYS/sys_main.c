/*
 * sys_main.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "stm32h7xx_hal.h"
#include "main.h"
#include "sys_main.h"
#include "sys_error.h"
#include "sys_sensor_timer.h"
#include "sys_temperature.h"
#include "sys_led.h"
#include "sleep/sleep_manager.h"
#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "ws2812b-driver.h"
#include "lps22hh-driver.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

osEventFlagsId_t sleep_events = NULL;
static volatile uint8_t hardware_id = 255;

/* Private function prototypes -----------------------------------------------*/

bool registerSysParam();
bool createSleepEvents();
void readHardwareId();

/* Exported function definitions ---------------------------------------------*/

void SYS_StartTask(void* argument)
{
  (void)(argument);
  if (Param_RegisterTask(SYS_TASK, "SYS") == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (registerSysParam() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (Param_TaskRegistrationComplete(SYS_TASK) == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  CFG_WaitLoadComplete();

  if (LPS_Init(LPS_ODR_1) == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (SensorTimer_Init() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (Pressure_Init() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (Temperature_Init() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (createSleepEvents() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  readHardwareId();

  for (;;) {
    LED_Update();
    Temperature_Process();
    SleepManager_Enter();
    osDelay(100);
  }
}

/* Private function definitions ----------------------------------------------*/

bool registerSysParam()
{
  if (LED_RegisterParams() == false) {
    return false;
  }
  return true;
}

bool createSleepEvents()
{
  sleep_events = osEventFlagsNew(NULL);

  return sleep_events != NULL;
}

void readHardwareId()
{
  hardware_id = 0;
  uint8_t bit = HAL_GPIO_ReadPin(HW_ID_PIN0_GPIO_Port, HW_ID_PIN0_Pin);
  hardware_id |= bit << 0;
  bit = HAL_GPIO_ReadPin(HW_ID_PIN1_GPIO_Port, HW_ID_PIN1_Pin);
  hardware_id |= bit << 1;
  bit = HAL_GPIO_ReadPin(HW_ID_PIN2_GPIO_Port, HW_ID_PIN2_Pin);
  hardware_id |= bit << 2;
  bit = HAL_GPIO_ReadPin(HW_ID_PIN3_GPIO_Port, HW_ID_PIN3_Pin);
  hardware_id |= bit << 3;
}
