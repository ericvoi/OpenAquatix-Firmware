/*
 * mess_dac_resources.h
 *
 *  Created on: Apr 29, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMMON_MESS_DAC_RESOURCES_H_
#define COMMON_MESS_DAC_RESOURCES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "dac_waveform.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes mutex for shared DAC and MESS resources
 */
void MessDacResource_Init(void);

/**
 * @brief Stores the configuration and bit message
 *
 * @param new_cfg Configuration structure with DSP parameters
 * @param new_bit_msg Bit message that is to be sent out
 *
 * @note pointers are not checked
 */
void MessDacResource_RegisterMessageConfiguration(const DspConfig_t* new_cfg,
    BitMessage_t* new_bit_msg);

/**
 * @brief Registers a tone to send with the DAC through feedback
 *
 * @param freq_hz Frequency of the tone
 * @param duration_ms Duration of the tone in ms
 * @param amplitude Amplitude of the signal relative to full-scale
 */
void MessDacResource_RegisterTestTone(uint32_t freq_hz, uint32_t duration_ms, float amplitude);

/**
 * @brief Registers a stair-stepped LFM chirp output
 *
 * Each call to MessDacResource_GetStep returns a constant-frequency
 * Symbol_t whose freq_hz advances linearly across num_steps steps.
 * The waveform engine carries the phase accumulator across steps so the
 * sweep is phase-continuous (only the instantaneous frequency is piecewise
 * constant). Used by mess_chirp.c for the paper-experiments TX probe.
 *
 * @param f_start_hz       Frequency of step 0
 * @param f_end_hz         Frequency at the end of the chirp
 * @param duration_us      Duration of the full chirp
 * @param amplitude        Amplitude relative to full-scale (0.0–1.0)
 */
void MessDacResource_RegisterChirp(uint32_t f_start_hz, uint32_t f_end_hz,
                                   uint32_t duration_us, float amplitude);

/**
 * @brief Get the next waveform step
 *
 * @param current_step Current step in the bit message
 *
 * @return structure with the frequency, duration, and amplitude to transmit
 */
Symbol_t MessDacResource_GetStep(uint16_t current_step);

/**
 * @brief Number of steps in the synchronization + wakeup sequence
 * 
 * Useful when the registered configuration is needed
 * 
 * @return uint16_t Number of steps
 */
uint16_t MessDacResource_SyncWakeupSteps(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_MESS_DAC_RESOURCES_H_ */
