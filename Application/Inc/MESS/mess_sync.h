/*
 * mess_sync.h
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_SYNC_H_
#define MESS_MESS_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_dsp_config.h"
#include "mess_packet.h"
#include "dac_waveform.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Sync_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, bool* bit, uint16_t step);
uint16_t Sync_NumSteps(const DspConfig_t* cfg);
bool Sync_Synchronize(const DspConfig_t* cfg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_SYNC_H_ */
