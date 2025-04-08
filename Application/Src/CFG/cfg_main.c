/*
 * cfg_main.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_main.h"
#include "cmsis_os.h"
#include "cfg_parameters.h"
#include "sys_error.h"
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

/* Private function prototypes -----------------------------------------------*/

static void waitAllTasksRegistered(void);
static bool registerCfgParams(void);
static bool waitForFlashSave(void);

/* Exported function definitions ---------------------------------------------*/

void CFG_StartTask(void* argument)
{
  (void)(argument);
  if (Param_RegisterTask(CFG_TASK, "CFG") == false) {
    Error_Routine(ERROR_CFG_INIT);
  }

  if (registerCfgParams() == false) {
    Error_Routine(ERROR_CFG_INIT);
  }

  if (Param_TaskRegistrationComplete(CFG_TASK) == false) {
    Error_Routine(ERROR_CFG_INIT);
  }
  // wait until all tasks parameters have been registered
  waitAllTasksRegistered();

  // then update all parameters from flash
  Param_LoadInit();
  osEventFlagsClear(flash_events, FLASH_SAVE_REQUESTED); // TODO: handle errors

  // then indicate to tasks that all parameters have been updated from flash memory
  osEventFlagsSet(param_events, EVENT_PARAMS_LOADED);
  for (;;) {
    if (waitForFlashSave() == true) {
      if (Param_SaveToFlash() == false) {
        Error_Routine(ERROR_FLASH);
      }
    }
    else {
      Error_Routine(ERROR_FLASH);
    }
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

/* Private function definitions ----------------------------------------------*/

void waitAllTasksRegistered()
{
  osEventFlagsWait(param_events, EVENT_ALL_TASKS_REGISTERED, osFlagsWaitAny, osWaitForever);
}

bool registerCfgParams()
{
  return true;
}

bool waitForFlashSave()
{
  uint32_t flags = osEventFlagsWait(flash_events, FLASH_SAVE_REQUESTED, osFlagsWaitAny, osWaitForever);
  if (flags & osFlagsError) {
    return false;
  }
  return true;
}
