/*
 * mess_hil_cal.c
 *
 *  Created on: Apr 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_hil_cal.h"
#include "hil_manager.h"
#include "hil_main.h"
#include "mess_dsp_config.h"
#include "mess_filt_resources.h"
#include "mess_modulate.h"
#include "mess_background_noise.h"
#include "mess_sync.h"
#include "filt_main.h"
#include "goertzel.h"
#include "feedback.h"
#include "math.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define CALIBRATION_ATTENUATION           ATTENUATION_93DB
#define LOOPBACK_TONE_DURATION_MS         200
#define LOOPBACK_TONE_AMPLITUDE           (0.2f)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static HilCalibrationPacket_t hil_cal;
static bool cal_ready = false;

extern osThreadId_t hil_taskHandle;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void HilCal_Perform(const DspConfig_t* cfg)
{
  cal_ready = false;
  memset(&hil_cal, 0, sizeof(HilCalibrationPacket_t));
  
  hil_cal.num_input_attenuations = NUM_IN_FB_ATTENUATIONS;
  hil_cal.input_attenuation[0] = 20.0f * log10f(Feedback_GetAttenuation(0));
  hil_cal.input_attenuation[1] = 20.0f * log10f(Feedback_GetAttenuation(1));
  hil_cal.output_attenuation =   20.0f * log10f(OUT_FB_ATTENUATION);

  // Modem operating center frequency. Authoritative source for both the host's
  // path-loss frequency (when this modem is the source) and noise-PSD reference
  // frequency (when this modem is the receiver).
  hil_cal.center_freq_hz = (float)cfg->fc;

//   Get noise floor when feedback connected
  BackgroundNoise_Reset(cfg);
  while (BackgroundNoise_Ready() == false) {
    BackgroundNoise_Calculate();
    MessFiltResources_SetProcessingTail(MessFiltResources_GetInputAdcHead());
    osDelay(1);
  }
  hil_cal.noise_floor_psd_counts_per_sqrt_hz =
      BackgroundNoise_GetNoiseFloorPsdCountsPerSqrtHz();

  // Calculate loopback gain
  FeedbackAttenuation_t atten = CALIBRATION_ATTENUATION;
  hil_cal.loopback_cal_attenuation = atten;
  Feedback_ChangeInputAttenuation(atten);

  Modulate_TestFrequencyResponse(cfg->fc, LOOPBACK_TONE_DURATION_MS, LOOPBACK_TONE_AMPLITUDE);
  osDelay(LOOPBACK_TONE_DURATION_MS / 4);
  MessFiltResources_SetProcessingTail(MessFiltResources_GetInputAdcHead());
  uint16_t samples_in_test = FILT_GetSamplingRate() * (LOOPBACK_TONE_DURATION_MS / 2) / 1000;
  while (MessFiltResources_AvailableProcessingSamples() < samples_in_test) osDelay(1);
  GoertzelInfo_t goertzel_info;
  uint32_t f[1] = {cfg->fc};
  goertzel_info.f = f;
  goertzel_info.buf_len = PROCESSING_BUFFER_SIZE;
  goertzel_info.data_len = samples_in_test;
  goertzel_info.start_pos = MessFiltResources_GetProcessingTail();
  float window[1] = {1.0f};
  goertzel_info.window = window;
  goertzel_info.window_size = 1;
  goertzel_info.energy_normalization = 1.0f;
  float e_f;
  goertzel_info.e_f = &e_f;
  goertzel_1(&goertzel_info);

  float adc_amplitude = 2.0f * sqrtf(goertzel_info.e_f[0] / goertzel_info.data_len);

  hil_cal.loopback_gain = adc_amplitude / LOOPBACK_TONE_AMPLITUDE;

  cal_ready = true;
}

bool HilCal_Get(HilCalibrationPacket_t* cal_packet)
{
  memcpy(cal_packet, &hil_cal, sizeof(HilCalibrationPacket_t));

  return cal_ready;
}

/* Private function definitions ----------------------------------------------*/
