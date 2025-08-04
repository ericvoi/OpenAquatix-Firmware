/*
 * wakeup_tones.c
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sleep/wakeup_tones.h"
#include "mess_dsp_config.h"
#include "dac_waveform.h"
#include "mess_modulate.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define NUM_WAKEUP_TONES        3
#define SILENCE_DURATION_MS     400

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void fhbfskWakeupStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index);
uint32_t fhbfskWakeupTone(const DspConfig_t* cfg, uint16_t step_index);
void fixedWakeupStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index);
uint32_t fixedWakeupTone(const DspConfig_t* cfg, uint16_t step_index);


/* Exported function definitions ---------------------------------------------*/

bool WakeupTones_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index)
{
  if (cfg->wakeup_tones == false || (step_index > NUM_WAKEUP_TONES)) {
    return false;
  }

  if (cfg->mod_demod_method == MOD_DEMOD_FHBFSK) {
    fhbfskWakeupStep(cfg, waveform_step, step_index);
  }
  else {
    fixedWakeupStep(cfg, waveform_step, step_index);
  }
  return true;
}

uint16_t WakeupTones_NumSteps(const DspConfig_t* cfg)
{
  if (cfg->wakeup_tones == true) {
    return NUM_WAKEUP_TONES + 1; // + 1 for silence period
  }
  else {
    return 0;
  }
}

/* Private function definitions ----------------------------------------------*/

void fhbfskWakeupStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index)
{
  if (step_index == NUM_WAKEUP_TONES) {
    waveform_step->relative_amplitude = 0.0f;
    waveform_step->duration_us = SILENCE_DURATION_MS;
    return;
  }

  waveform_step->freq_hz = fhbfskWakeupTone(cfg, step_index);
  waveform_step->duration_us = (uint32_t) 4 * (1000000.0f / cfg->baud_rate);
  waveform_step->relative_amplitude = Modulate_GetAmplitude(waveform_step->freq_hz);
}

uint32_t fhbfskWakeupTone(const DspConfig_t* cfg, uint16_t step_index)
{
  if (step_index > NUM_WAKEUP_TONES) {
    return 0;
  }

  DspConfig_t temp_cfg;
  memcpy(&temp_cfg, cfg, sizeof(DspConfig_t));
  temp_cfg.fhbfsk_hopper = HOPPER_INCREMENT;

  // Calculate the actual center frequency since the one in cfg is not necessarily
  // aligned with a DFT bin
  uint16_t bit_index = cfg->fhbfsk_num_tones / 2;
  bool bit = cfg->fhbfsk_num_tones % 2;
  uint32_t center_freq = Modulate_GetFhbfskFrequency(bit, bit_index, &temp_cfg);

  uint32_t bandwidth = cfg->baud_rate * (cfg->fhbfsk_num_tones * 2 - 1) * cfg->fhbfsk_freq_spacing;
  int16_t offset = step_index - 1;
  return center_freq - bandwidth * (offset) / 2; // Fc - BW/2, Fc, Fc + BW/2
}

void fixedWakeupStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index)
{
  waveform_step->freq_hz = fixedWakeupTone(cfg, step_index);

  waveform_step->duration_us = (uint32_t) 4 * (1000000.0f / cfg->baud_rate);
}

uint32_t fixedWakeupTone(const DspConfig_t* cfg, uint16_t step_index)
{
  switch (step_index) {
    case 0:
      return cfg->wakeup_tone1;
    case 1:
      return cfg->wakeup_tone2;
    case 2:
      return cfg->wakeup_tone3;
    default:
      return 0;
  }
}
