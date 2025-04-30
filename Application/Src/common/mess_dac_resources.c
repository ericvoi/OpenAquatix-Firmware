/*
 * mess_dac_resources.c
 *
 *  Created on: Apr 29, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_dac_resources.h"
#include "mess_dsp_config.h"
#include "mess_modulate.h"
#include "dac_waveform.h"
#include "sys_error.h"
#include "cmsis_os.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define MUTEX_TIMEOUT   0 // No timeout since it must be instant

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static osMutexId_t mess_dac_resource_mutex;

static DspConfig_t cfg;
static BitMessage_t bit_msg;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void MessDacResource_Init()
{
  mess_dac_resource_mutex = osMutexNew(NULL);
}

void MessDacResource_RegisterMessageConfiguration(const DspConfig_t* new_cfg,
    BitMessage_t* new_bit_msg)
{
  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    Error_Routine(ERROR_MESS_DAC_RESOURCE);
    return;
  }
  memcpy(&cfg, new_cfg, sizeof(DspConfig_t));
  memcpy(&bit_msg, new_bit_msg, sizeof(BitMessage_t));

  osMutexRelease(mess_dac_resource_mutex);
}

WaveformStep_t MessDacResource_GetStep(uint16_t current_step)
{
  bool bit;
  WaveformStep_t waveform_step = {0};
  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    Error_Routine(ERROR_MESS_DAC_RESOURCE);
    return waveform_step;
  }
  if (Packet_GetBit(&bit_msg, current_step, &bit) == false) {
    return waveform_step;
  }
  switch (cfg.mod_demod_method) {
    case MOD_DEMOD_FSK:
      waveform_step.freq_hz = Modulate_GetFskFrequency(bit, &cfg);
      break;
    case MOD_DEMOD_FHBFSK:
      waveform_step.freq_hz = Modulate_GetFhbfskFrequency(bit, current_step, &cfg);
      break;
    default:
      osMutexRelease(mess_dac_resource_mutex);
      return waveform_step;
  }
  waveform_step.relative_amplitude = Modulate_GetAmplitude(waveform_step.freq_hz);
  waveform_step.duration_us = (uint32_t) roundf(1000000.0f / cfg.baud_rate);
  osMutexRelease(mess_dac_resource_mutex);
  return waveform_step;
}

/* Private function definitions ----------------------------------------------*/
