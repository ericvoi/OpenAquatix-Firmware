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
 * @brief Initializes events and queues in background noise module
 * 
 * @return true if successfully made os structures, false otherwise
 */
bool BackgroundNoise_Init();

/**
 * @brief Restarts the background noise and invalidates the current background
 * noise measurement
 */
void BackgroundNoise_Reset();

/**
 * @brief Calculates the background noise in band on the most recent data
 * 
 * @return true if successfully updated/processed background noise, false otherwise
 */
bool BackgroundNoise_Calculate(const DspConfig_t* cfg);

/**
 * @brief Returns the calculated background noise
 * 
 * @return float Background noise (scaleless)
 */
float BackgroundNoise_Get();

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
