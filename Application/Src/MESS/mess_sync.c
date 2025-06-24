/*
 * mess_sync.c
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_sync.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "dac_waveform.h"
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static const uint32_t janus_pn_32 = 0b10101110110001111100110100100000U;

/* Private function prototypes -----------------------------------------------*/

static bool janusPnStep(bool* bit, uint16_t step);

/* Exported function definitions ---------------------------------------------*/

bool Sync_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, bool* bit, uint16_t step)
{
  (void)(waveform_step); // Planned use for advanced synchronization techniques
  switch (cfg->sync_method) {
    case NO_SYNC:
      return false;
    case SYNC_PN_32_JANUS:
      return janusPnStep(bit, step);
    default:
      return false;
  }
  return true;
}

uint16_t Sync_NumSteps(const DspConfig_t* cfg)
{
  switch (cfg->sync_method) {
    case NO_SYNC:
      return 0;
    case SYNC_PN_32_JANUS:
      return 32;
    default:
      return false;
  }
}

bool Sync_Synchronize(const DspConfig_t* cfg)
{
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool janusPnStep(bool* bit, uint16_t step)
{
  if (step >= 32) return false;
  *bit = (janus_pn_32 >> (31 - step)) & 1;
  return true;
}
