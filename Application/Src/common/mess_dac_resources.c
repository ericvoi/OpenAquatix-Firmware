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
#include "mess_sync.h"
#include "dac_waveform.h"
#include "sys_error.h"
#include "sleep/wakeup_tones.h"
#include "cmsis_os.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

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

/* Private function prototypes -----------------------------------------------*/

TransmissionPhase_t getPhase(uint16_t current_step, uint16_t* transmission_step, uint16_t* symbol_index);

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

  transmission_layout.wakeup_steps = WakeupTones_NumSteps(new_cfg);
  transmission_layout.sync_steps = Sync_NumSteps(new_cfg);

  osMutexRelease(mess_dac_resource_mutex);
}

WaveformStep_t MessDacResource_GetStep(uint16_t current_step)
{
  WaveformStep_t waveform_step = {0};

  if (osMutexAcquire(mess_dac_resource_mutex, MUTEX_TIMEOUT) != osOK) {
    Error_Routine(ERROR_MESS_DAC_RESOURCE);
    return waveform_step;
  }

  uint16_t transmission_step;
  uint16_t symbol_step;
  TransmissionPhase_t transmission_phase = getPhase(current_step, &transmission_step, &symbol_step);

  bool success = false;
  switch (transmission_phase) {
    case PACKET_PHASE_WAKEUP:
      success = WakeupTones_GetStep(&cfg, &waveform_step, transmission_step);
      break;
    case PACKET_PHASE_SYNC:
      success = Sync_GetStep(&cfg, &waveform_step, transmission_step);
      break;
    case PACKET_PHASE_DATA:
      success = Modulate_DataStep(&cfg, &bit_msg, &waveform_step, transmission_step, symbol_step);
      break;
    default:
      success = false;
      break;
  }
  osMutexRelease(mess_dac_resource_mutex);
  if (success == false) {
    Error_Routine(ERROR_DAC_PROCESSING);
    return waveform_step;
  }
  return waveform_step;
}

uint16_t MessDacResource_SyncSteps()
{
  return Sync_NumSteps(&cfg);
}

/* Private function definitions ----------------------------------------------*/

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
