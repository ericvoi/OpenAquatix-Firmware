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

#define ALL_FLAGS (DAC_FILL_FIRST_HALF | DAC_FILL_LAST_HALF)

/* Private macro -------------------------------------------------------------*/

// check queue for latest waveform steps as well as set a flag when the message has been fully added

/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void registerDacParams(void);

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

  for (;;)
  {
    uint32_t flags;
    flags = osThreadFlagsWait(ALL_FLAGS, osFlagsWaitAny, osWaitForever);

    if (flags & DAC_FILL_FIRST_HALF) {
      Waveform_FillBuffer(FILL_FIRST_HALF);
    }

    if (flags & DAC_FILL_LAST_HALF) {
      Waveform_FillBuffer(FILL_LAST_HALF);
    }
  }
}

/* Private function definitions ----------------------------------------------*/

void registerDacParams()
{
  Waveform_RegisterParams();
}
