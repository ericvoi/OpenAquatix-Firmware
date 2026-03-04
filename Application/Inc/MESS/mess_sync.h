/*
 * mess_sync.h
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
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

typedef enum {
  SYNC_OK,
  SYNC_SUCCESS
} SyncState_t;

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
 * @brief Synchronizes the receiver and sender using either an amplitude detection
 * or a 32-chip JANUS preamble
 * 
 * @param cfg 
 * 
 * @return SYNC_OK if no errors and no synchronization yet
 * @return SYNC_SUCCESS if successfully synchronized
 */
SyncState_t Sync_Synchronize(const DspConfig_t* cfg);

/**
 * @brief Returns most recent snr score from synchronization
 * 
 * @return float The most recent synchronization snr score
 */
float Sync_MostRecentSnr();

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
