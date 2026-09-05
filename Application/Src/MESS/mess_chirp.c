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

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define MAX_CHIRP_TEMPLATE_SAMPLES      4800 // 200ms chirp at 24 kSPS

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static float chirp_template[MAX_CHIRP_TEMPLATE_SAMPLES];
static uint32_t template_samples = 0;

extern osThreadId_t dac_taskHandle;

/* Private function prototypes -----------------------------------------------*/

// Both use duration instead of samples as using samples creates a significant 
// difference between what is modulated and what is received. The chirp can be
// cut off but not scaled in time
static void populateLfmChirp(float duration_s, uint32_t f_start, uint32_t f_end, uint32_t f_s, float* buf);
static void populateHfmChirp(float duration_s, uint32_t f_start, uint32_t f_end, uint32_t f_s, float* buf);

/* Exported function definitions ---------------------------------------------*/

void Chirp_UpdateTemplate(float duration_s, ModulationOutputType_t type,
  uint32_t f_start, uint32_t f_end, uint32_t f_s)
{
  uint32_t n_samples = duration_s * f_s;
  if (n_samples > MAX_CHIRP_TEMPLATE_SAMPLES)
    REGISTER_ERROR(ERROR_INVALID_REFERENCE_CHIRP);

  template_samples = n_samples;

  switch (type) {
    case OUTPUT_LFM: 
      populateLfmChirp(duration_s, f_start, f_end, f_s, chirp_template);
      break;
    case OUTPUT_HFM: 
      populateHfmChirp(duration_s, f_start, f_end, f_s, chirp_template);
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
      break;
  }
}

void Chirp_TestChirp(void)
{
  RETURN_IF_ERROR_PRESENT();

  if (HAL_TIM_Base_Stop(&htim6) != HAL_OK)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  if (Waveform_StopWaveformOutput() == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  MessDacResource_RegisterChirp(CHIRP_F_START_HZ, CHIRP_F_END_HZ,
                                CHIRP_DURATION_US, CHIRP_AMPLITUDE);

  if (Waveform_SetWaveformSequence(1, false, false, 0) == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  if (Waveform_PrepareWaveformOutput(DAC_CHANNEL_1) == false)
    REGISTER_ERROR(ERROR_TRANSDUCER_FB_INITIALIZATION);

  osDelay(150);

  osThreadFlagsSet(dac_taskHandle, DAC_START_OUTPUT);
}

/* Private function definitions ----------------------------------------------*/

static void populateLfmChirp(float duration_s, uint32_t f_start, uint32_t f_end, uint32_t f_s, float* buf)
{
  float time_increment = 1.0f / f_s;
  float time = 0.0f;
  float freq_diff = (int32_t) f_end - (int32_t) f_start;
  float phase = 0.0f;
  uint16_t i = 0;
  float sample_freq = f_s;
  float start_freq = f_start;
  float chirp_freq = 1.0f / duration_s;
  while (time < duration_s) {
    buf[i] = sinf(phase);
    phase += 2 * M_PI * (((float) start_freq) + freq_diff * time * chirp_freq) / sample_freq;
    if (phase > 10.0f * M_PI) phase -= 10.0f * M_PI; // Prevent FP errors
    time += time_increment;
    i++;
    if (i > MAX_CHIRP_TEMPLATE_SAMPLES) {
      REGISTER_ERROR(ERROR_INVALID_REFERENCE_CHIRP);
    }
  }
}

static void populateHfmChirp(float duration_s, uint32_t f_start, uint32_t f_end, uint32_t f_s, float* buf)
{
  // TODO
}
