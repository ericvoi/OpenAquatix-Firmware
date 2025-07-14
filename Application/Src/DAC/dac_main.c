/*
 * dac_main.c
 *
 *  Created on: Apr 28, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/
#include "dac_main.h"
#include "dac_waveform.h"
#include "cfg_parameters.h"
#include "sys_error.h"
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

bool registerDacParams(void);

/* Exported function definitions ---------------------------------------------*/

void DAC_StartTask(void* argument)
{
  (void)(argument);
  if (Param_RegisterTask(DAC_TASK, "DAC") == false) {
    Error_Routine(ERROR_DAC_INIT);
  }

  if (registerDacParams() == false) {
    Error_Routine(ERROR_DAC_INIT);
  }

  if (Param_TaskRegistrationComplete(DAC_TASK) == false) {
    Error_Routine(ERROR_DAC_INIT);
  }

  CFG_WaitLoadComplete();

  Waveform_InitWaveformGenerator();

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

bool registerDacParams()
{
  if (Waveform_RegisterParams() == false) {
    return false;
  }
  return true;
}
