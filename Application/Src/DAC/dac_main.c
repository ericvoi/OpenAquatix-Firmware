/*
 * dac_main.c
 *
 *  Created on: Apr 28, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/
#include "dac_main.h"
#include "dac_waveform.h"
#include "cfg_parameters.h"
#include "error_manager.h"
#include "cfg_main.h"
#include "main.h"
#include "cmsis_os.h"
#include <stdbool.h>


/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void registerDacParams(void);
static void resetTask(void);

/* Exported function definitions ---------------------------------------------*/

void DAC_StartTask(void* argument)
{
  (void)(argument);
  Error_RegisterTask("DAC");
  registerDacParams();
  Error_ParameterRegistrationComplete();
  CFG_WaitLoadComplete();

  Waveform_InitWaveformGenerator();

  // TODO: address why this is needed twice
  Waveform_Flush();

  resetTask();

  for (;;)
  {
    uint32_t flags;
    flags = osThreadFlagsWait(0xFF, osFlagsWaitAny, osWaitForever);

    if (flags & DAC_FILL_FIRST_HALF) {
      Waveform_FillBuffer(FILL_FIRST_HALF);
    }

    if (flags & DAC_FILL_LAST_HALF) {
      Waveform_FillBuffer(FILL_LAST_HALF);
    }

    if (flags & DAC_START_OUTPUT) {
      Waveform_StartOutput();
    }

    if (flags & DAC_START_RANGING_REQUEST) {
      Waveform_SendRangingRequest();
    }

    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
  }
}

/* Private function definitions ----------------------------------------------*/

void registerDacParams()
{
  Waveform_RegisterParams();
}

void resetTask()
{

}
