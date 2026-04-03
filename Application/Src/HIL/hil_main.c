/*
 * hil_main.c
 *
 *  Created on: Apr 2, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "hil_main.h"
#include "error_manager.h"
#include "cfg_main.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void resetTask(void);

/* Exported function definitions ---------------------------------------------*/

void HIL_StartTask(void* argument)
{
  (void) (argument);
  Error_RegisterTask("HIL");
  Error_ParameterRegistrationComplete();

  CFG_WaitLoadComplete();

  resetTask();
  for (;;) {
    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
    osDelay(1);
  }
}

/* Private function definitions ----------------------------------------------*/

static void resetTask(void)
{

}
