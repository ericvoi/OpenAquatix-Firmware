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



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

osEventFlagsId_t param_events = NULL;

/* Private function prototypes -----------------------------------------------*/

void waitAllTasksRegistered(void);
bool registerCfgParams();

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

  // then indicate to tasks that all parameters have been updated from flash memory
  osEventFlagsSet(param_events, EVENT_PARAMS_LOADED);
  for (;;) {
    osDelay(10);
  }
}

bool CFG_CreateParamFlags(void)
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
  return true;
}

void CFG_WaitLoadComplete(void)
{
  osEventFlagsWait(param_events, EVENT_PARAMS_LOADED, osFlagsNoClear, osWaitForever);
}

/* Private function definitions ----------------------------------------------*/

void waitAllTasksRegistered(void)
{
  osEventFlagsWait(param_events, EVENT_ALL_TASKS_REGISTERED, osFlagsWaitAny, osWaitForever);
}

bool registerCfgParams()
{
  return true;
}
