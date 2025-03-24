/*
 * sys_main.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_main.h"
#include "main.h"
#include "sys_error.h"
#include "cfg_main.h"
#include "cfg_parameters.h"
#include "WS2812b-driver.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

bool registerSysParam();

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

  for (;;) {
    WS_Update();
    osDelay(1000);
  }
}

/* Private function definitions ----------------------------------------------*/

bool registerSysParam()
{
  return true;
}
