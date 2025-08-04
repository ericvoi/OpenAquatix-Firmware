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

/**
 * @brief Returns the necessary information for modulating a synchronization
 * sequence
 * 
 * @param cfg Configuration struct to use
 * @param waveform_step Parameters of the waveform to use for modulation (modified)
 * @param step current step
 * @return true if successful, false otherwise
 */
bool Sync_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step);

/**
 * @brief Returns number of steps in the synchronization sequence
 * 
 * @param cfg Configuration struct to use
 * @return uint16_t Number of steps in the synchronization sequence
 */
uint16_t Sync_NumSteps(const DspConfig_t* cfg);

/**
 * @brief Synchronizes the receiver and sender (NOT IMPLEMENTED YET)
 * 
 * @param cfg 
 * @return true 
 * @return false 
 */
bool Sync_Synchronize(const DspConfig_t* cfg);

/**
 * @brief Resets the synchronization process
 * 
 * To be called when a message has been started to reset
 */
void Sync_Reset();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_SYNC_H_ */
