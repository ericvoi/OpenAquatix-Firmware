/*
 * mess_chirp.c
 *
 *  Created on: May 8, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "mess_chirp.h"
#include "mess_dac_resources.h"
#include "dac_waveform.h"
#include "dac_main.h"
#include "mess_main.h"
#include "cmsis_os.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private variables ---------------------------------------------------------*/

extern osThreadId_t dac_taskHandle;

/* Exported function definitions ---------------------------------------------*/

void MessChirp_StartTx(void)
{
  RETURN_IF_ERROR_PRESENT();

  HAL_TIM_Base_Stop(&htim6);
  if (Waveform_StopWaveformOutput() == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  osDelay(1);

  MessDacResource_RegisterChirp(CHIRP_F_START_HZ, CHIRP_F_END_HZ,
                                CHIRP_NUM_STEPS, CHIRP_STEP_DURATION_US,
                                CHIRP_AMPLITUDE);

  if (Waveform_SetWaveformSequence(CHIRP_NUM_STEPS, false, false, 0) == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  if (Waveform_PrepareWaveformOutput(DAC_CHANNEL_1) == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  osThreadFlagsSet(dac_taskHandle, DAC_START_OUTPUT);
}
