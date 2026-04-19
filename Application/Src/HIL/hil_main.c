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
#include "hil_manager.h"
#include "hil_stream.h"
#include "hil_buffer.h"
#include "cfg_main.h"
#include "cmsis_os.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void handleThreadFlags(void);
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
    handleThreadFlags();
    if (Error_CheckModuleReset() == TASK_RESET) {
      resetTask();
    }
    Error_ResetAbortFlag();
  }
}

/* Private function definitions ----------------------------------------------*/

static void handleThreadFlags(void)
{
  uint32_t flags = osThreadFlagsWait(HIL_EVT_ALL, osFlagsWaitAny, osWaitForever);

  if (flags & osFlagsError) {
    REGISTER_ERROR(ERROR_FLAGS_RUNNING);
    return;
  }

  if (flags & HIL_EVT_STREAM_RX_RDY) HilBuf_ReadRxPackets();

  if (flags & HIL_EVT_STREAM_TX_CPLT) HilBuf_SendTxPackets();

  if (flags & HIL_EVT_ADC_HALF_FULL) HilStream_AdcCallback(false);

  if (flags & HIL_EVT_ADC_FULL) HilStream_AdcCallback(true);

  if (flags & HIL_EVT_DAC_HALF_FULL) HilStream_DacCallback(false);

  if (flags & HIL_EVT_DAC_FULL) HilStream_DacCallback(true);

  if (flags & HIL_EVT_CONTROL_CMD) HilManager_ProcessCommand();

  if (flags & HIL_EVT_CAL_DONE) HilManager_CalibrationDone();

  if (flags & HIL_EVT_ENTER_TRANSITION) HilManager_SetState(HIL_STATE_TRANSITIONING);

  if (flags & HIL_EVT_ENTER_RX) HilManager_SetState(HIL_STATE_RX);

  if (flags & HIL_EVT_ENTER_TX) HilManager_SetState(HIL_STATE_TX);
}

static void resetTask(void)
{

}
