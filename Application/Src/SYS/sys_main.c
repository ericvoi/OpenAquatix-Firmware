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
#include "sys_sensor_timer.h"
#include "sys_temperature.h"
#include "sys_pressure.h"
#include "sys_led.h"
#include "sys_power.h"
#include "sleep/sleep_manager.h"
#include "error_manager.h"
#include "error_subsys.h"
#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "ws2812b-driver.h"
#include "lps22hh-driver.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define LPS_READ_INTERVAL_MS        (100)
#define DEFAULT_LPS_ODR             (LPS_ODR_1)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

osEventFlagsId_t sleep_events = NULL;
static volatile uint8_t hardware_id = 255;
static uint32_t last_lps_read = 0;

/* Private function prototypes -----------------------------------------------*/

static void registerSysParams(void);
static void createSleepEvents(void);
static void readHardwareId(void);
static void readLps(void);
static void resetTask(void);

/* Exported function definitions ---------------------------------------------*/

void SYS_StartTask(void* argument)
{
  (void)(argument);
  Error_RegisterTask("SYS");
  registerSysParams();
  Error_ParameterRegistrationComplete();

  LPS_CreateResources();
  createSleepEvents();
  
  CFG_WaitLoadComplete();

  readHardwareId();
  resetTask();

  for (;;) {
    LED_Update();
    Temperature_Process();
    Power_Process();
    readLps();
    Pressure_Process();
    SleepManager_Enter();

    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
    osDelay(5);
  }
}

/* Private function definitions ----------------------------------------------*/

void registerSysParams(void)
{
  LED_RegisterParams();
}

void createSleepEvents(void)
{
  if (sleep_events != NULL) REGISTER_ERROR(ERROR_FLAGS_INITIALIZATION);

  sleep_events = osEventFlagsNew(NULL);

  if (sleep_events == NULL) REGISTER_ERROR(ERROR_FLAGS_INITIALIZATION);
}

void readHardwareId(void)
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

void readLps(void)
{
  RETURN_IF_ERROR_PRESENT();
  SubSystemStatus_t lps_status = ErrorSubsys_CurrentStatus(SUBSYS_LPS);
  uint32_t current_timestamp = HAL_GetTick();

  if (lps_status == SUBSYS_DISABLE) {
    LPS_PowerDown();
    return;
  }
  if (lps_status == SUBSYS_RESET) {
    LPS_Init(DEFAULT_LPS_ODR);
    ErrorSubsys_ClearReset(SUBSYS_LPS);
    return;
  }

  if (current_timestamp - last_lps_read < LPS_READ_INTERVAL_MS) {
    LPS_ReadData();
    last_lps_read = current_timestamp;
  }
}

void resetTask(void)
{
  LPS_Init(DEFAULT_LPS_ODR);
  SensorTimer_Init();
  Pressure_Init();
  Temperature_Init();
  Power_Init();
}
