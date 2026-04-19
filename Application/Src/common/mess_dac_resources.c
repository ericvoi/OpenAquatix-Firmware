/*
 * mess_dac_resources.c
 *
 *  Created on: Apr 29, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_dac_resources.h"
#include "mess_dsp_config.h"
#include "mess_modulate.h"
#include "mess_sync.h"
#include "dac_waveform.h"
#include "error_manager.h"
#include "sleep/wakeup_tones.h"
#include "cmsis_os.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  OUTPUT_MESSAGE,
  OUTPUT_TEST_TONE
} DacOutputType_t;

typedef enum {
  PACKET_PHASE_WAKEUP,
  PACKET_PHASE_SYNC,
  PACKET_PHASE_DATA
} TransmissionPhase_t;

typedef struct {
  uint16_t wakeup_steps;
  uint16_t sync_steps;
} TransmissionLayout_t;

/* Private define ------------------------------------------------------------*/

#define MUTEX_TIMEOUT   0 // No timeout since it must be instant

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static osMutexId_t mess_dac_resource_mutex;

static DspConfig_t cfg;
static BitMessage_t bit_msg;

static TransmissionLayout_t transmission_layout;
static DacOutputType_t output_type;
static WaveformStep_t test_tone;

/* Private function prototypes -----------------------------------------------*/

WaveformStep_t getMessageStep(uint16_t current_step);
WaveformStep_t getTestToneStep(uint16_t current_step);
TransmissionPhase_t getPhase(uint16_t current_step, uint16_t* transmission_step, uint16_t* symbol_index);

/* Exported function definitions ---------------------------------------------*/

void MessDacResource_Init()
{
  if (mess_dac_resource_mutex != NULL) 
    REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);
  mess_dac_resource_mutex = osMutexNew(NULL);
  if (mess_dac_resource_mutex == NULL) 
    REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);
}

void MessDacResource_RegisterMessageConfiguration(const DspConfig_t* new_cfg,
    BitMessage_t* new_bit_msg)
{
  RETURN_IF_ERROR_PRESENT();
  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    REGISTER_ERROR(ERROR_MUTEX_TIMEOUT);
    return;
  }
  memcpy(&cfg, new_cfg, sizeof(DspConfig_t));
  memcpy(&bit_msg, new_bit_msg, sizeof(BitMessage_t));

  transmission_layout.wakeup_steps = WakeupTones_NumSteps(new_cfg);
  transmission_layout.sync_steps = Sync_NumSteps(new_cfg);
  output_type = OUTPUT_MESSAGE;

  osMutexRelease(mess_dac_resource_mutex);
}

void MessDacResource_RegisterTestTone(uint32_t freq_hz, uint32_t duration_ms, float amplitude)
{
  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    REGISTER_ERROR(ERROR_MUTEX_TIMEOUT);
    return;
  }

  test_tone.freq_hz = freq_hz;
  test_tone.duration_us = duration_ms * 1000;
  test_tone.relative_amplitude = amplitude;
  test_tone.output_type = OUTPUT_CONSTANT_SQUARE;
  output_type = OUTPUT_TEST_TONE;

  osMutexRelease(mess_dac_resource_mutex);
}

WaveformStep_t MessDacResource_GetStep(uint16_t current_step)
{
  switch (output_type) {
    case OUTPUT_MESSAGE:
      return getMessageStep(current_step);
    case OUTPUT_TEST_TONE:
      return getTestToneStep(current_step);
    default: {
      WaveformStep_t step = {0};
      REGISTER_ERROR_NON_VOID(ERROR_UNHANDLED_CASE, step);
      return step;
    }
  }
}

uint16_t MessDacResource_SyncWakeupSteps()
{
  uint16_t sync_steps = Sync_NumSteps(&cfg);
  uint16_t wakeup_steps = WakeupTones_NumSteps(&cfg);
  return sync_steps + wakeup_steps;
}

/* Private function definitions ----------------------------------------------*/

WaveformStep_t getMessageStep(uint16_t current_step)
{
  WaveformStep_t waveform_step = {0};

  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    REGISTER_ERROR_NON_VOID(ERROR_MUTEX_TIMEOUT, waveform_step);
    return waveform_step;
  }

  uint16_t transmission_step;
  uint16_t symbol_step;
  TransmissionPhase_t transmission_phase = getPhase(current_step, &transmission_step, &symbol_step);

  switch (transmission_phase) {
    case PACKET_PHASE_WAKEUP:
      WakeupTones_GetStep(&cfg, &waveform_step, transmission_step);
      break;
    case PACKET_PHASE_SYNC:
      Sync_GetStep(&cfg, &waveform_step, transmission_step);
      break;
    case PACKET_PHASE_DATA:
      Modulate_DataStep(&cfg, &bit_msg, &waveform_step, transmission_step, symbol_step);
      break;
    default:
      REGISTER_ERROR_NON_VOID(ERROR_UNHANDLED_CASE, waveform_step);
      break;
  }
  osMutexRelease(mess_dac_resource_mutex);
  return waveform_step;
}

WaveformStep_t getTestToneStep(uint16_t current_step)
{
  WaveformStep_t waveform_step = {0};
  if (current_step != 0) return waveform_step;

  return test_tone;
}

TransmissionPhase_t getPhase(uint16_t current_step, uint16_t* transmission_step, uint16_t* symbol_index)
{
  if (current_step < transmission_layout.wakeup_steps) {
    *transmission_step = current_step;
    return PACKET_PHASE_WAKEUP;
  }

  if (current_step < (transmission_layout.wakeup_steps + transmission_layout.sync_steps)) {
    *transmission_step = current_step - transmission_layout.wakeup_steps;
    return PACKET_PHASE_SYNC;
  }

  *transmission_step = current_step - (transmission_layout.wakeup_steps + transmission_layout.sync_steps);
  *symbol_index = current_step - transmission_layout.wakeup_steps;
  return PACKET_PHASE_DATA;
}
