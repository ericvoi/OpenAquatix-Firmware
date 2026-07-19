/*
 * mess_background_noise.h
 *
 *  Created on: Jul 14, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_BACKGROUND_NOISE_H_
#define MESS_MESS_BACKGROUND_NOISE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_dsp_config.h"
#include <stdint.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes events and queues for background noise
 * 
 * @note Can register a fatal error if initialization failed
 */
void BackgroundNoise_CreateShared();

/**
 * @brief Restarts the background noise and invalidates the current background
 * noise measurement
 * 
 * @param cfg DSP configuration for noise calculations
 */
void BackgroundNoise_Reset(const DspConfig_t* cfg);

/**
 * @brief Calculates the background noise in band on the most recent data
 */
void BackgroundNoise_Calculate(void);

/**
 * @brief Returns the calculated background noise
 * 
 * @return float Background noise (scaleless)
 */
float BackgroundNoise_GetScaleless();

/**
 * @brief Returns the calculated background noise seen by ADC
 * 
 * @return float Noise Spectral Density in nV / sqrt(Hz)
 */
float BackgroundNoise_GetNsd();

/**
 * @brief Returns the AFE noise floor as a one-sided power spectral density
 *        amplitude, in ADC counts per sqrt(Hz). This is the unit the host
 *        consumes; it lets the host compare AFE noise to the simulator's
 *        Wenz ambient PSD without negotiating measurement bandwidth.
 *
 * @return float counts/sqrt(Hz), or 0.0f if the estimator has not yet
 *         produced a valid result.
 */
float BackgroundNoise_GetNoiseFloorPsdCountsPerSqrtHz(void);

/**
 * @brief Whether enough samples have been analyzed for a background noise calculation
 * 
 * @return true if ready, false otherwise
 */
bool BackgroundNoise_Ready();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_BACKGROUND_NOISE_H_ */
