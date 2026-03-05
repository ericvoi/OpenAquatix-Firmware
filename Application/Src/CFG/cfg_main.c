/*
 * cfg_main.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_main.h"
#include "cmsis_os.h"
#include "cfg_parameters.h"
#include "error_manager.h"
#include "main.h"

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  FLASH_SAVE_REQUESTED = 0x00000001,
} FlashEvents_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

osEventFlagsId_t param_events = NULL;
static osEventFlagsId_t flash_events;
static volatile uint32_t cfg_number = 1;

/* Private function prototypes -----------------------------------------------*/

static void waitAllTasksRegistered(void);
static void registerCfgParams(void);
static void waitForFlashSave(void);
static void resetTask(void);

/* Exported function definitions ---------------------------------------------*/

void CFG_StartTask(void* argument)
{
  (void)(argument);

  Error_RegisterTask("CFG");
  registerCfgParams();
  Error_ParameterRegistrationComplete();
  waitAllTasksRegistered();

  // Update all parameters from flash
  Param_LoadInit();
  osEventFlagsClear(flash_events, FLASH_SAVE_REQUESTED);

  // then indicate to tasks that all parameters have been updated from flash memory
  osEventFlagsSet(param_events, EVENT_PARAMS_LOADED);
  for (;;) {
    waitForFlashSave();
    osDelay(100); // Small wait for multiple successive saves
    Param_SaveToFlash();

    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
  }
}

bool CFG_CreateFlags()
{
  static const osEventFlagsAttr_t event_attr = {
      .name = "ParamEvents",
      .attr_bits = 0,
      .cb_mem = NULL,
      .cb_size = 0
  };

  param_events = osEventFlagsNew(&event_attr);

  if (param_events == NULL) {
    return false;
  }

  static const osEventFlagsAttr_t flash_attr = {
      .name = "FlashEvents",
      .attr_bits = 0,
      .cb_mem = NULL,
      .cb_size = 0
  };

  flash_events = osEventFlagsNew(&flash_attr);

  if (flash_events == NULL) {
    return false;
  }
  return true;
}

void CFG_WaitLoadComplete()
{
  osEventFlagsWait(param_events, EVENT_PARAMS_LOADED, osFlagsNoClear, osWaitForever);
}

void CFG_SetFlashSaveFlag()
{
  osEventFlagsSet(flash_events, FLASH_SAVE_REQUESTED);
}

void CFG_IncrementVersionNumber()
{
  cfg_number++;
}

uint32_t CFG_GetVersionNumber()
{
  return cfg_number;
}

/* Private function definitions ----------------------------------------------*/

void waitAllTasksRegistered()
{
  osEventFlagsWait(param_events, EVENT_ALL_TASKS_REGISTERED, osFlagsWaitAny, osWaitForever);
}

void registerCfgParams()
{

}

void waitForFlashSave()
{
  uint32_t flags = osEventFlagsWait(flash_events, FLASH_SAVE_REQUESTED, osFlagsWaitAny, osWaitForever);
  if (flags & osFlagsError) 
    REGISTER_ERROR(ERROR_FLAGS_RUNNING);
}

void resetTask(void)
{

}
