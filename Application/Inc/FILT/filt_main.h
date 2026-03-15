/*
 * filt_main.h
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef FILT_FILT_MAIN_H_
#define FILT_FILT_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "cfg_parameters.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  FILT_FIRST_HALF_RDY_RAW   = 1 << 0,
  FILT_SECOND_HALF_RDY_RAW  = 1 << 1,
  FILT_FMAC_RDY             = 1 << 2
} FiltEvents_t;

#define DIGITAL_FILTER_TABLE(X) \
  X(DIGITAL_FILTER_DEC, "Decimation filter") \
  X(DIGITAL_FILTER_NONE, "No filter")

DECLARE_ENUM(DIGITAL_FILTER_TABLE, NUM_DIGITAL_FILTERS, DigitalFilter_t)

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Starts the FILT task
 * 
 * @param argument (ignored)
 */
void FILT_StartTask(void* argument);

/**
 * @brief Converts a baseband frequency into its passband counterpart
 * 
 * @param freq_hz Original baseband frequency
 * 
 * @return uint32_t Folded frequency
 * 
 * @note This must be called when referencing any frequency in the ADC buffers
 */
uint32_t FILT_PassbandToBaseband(uint32_t freq_hz);

/**
 * @brief Gets the bandwidth of the baseband input (after decimation)
 * 
 * @return uint32_t Baseband bandwidth
 */
uint32_t FILT_GetBandwidth(void);

/**
 * @brief Gets the effective sampling rate after decimation
 * 
 * @return uint32_t Effective sampling rate
 */
uint32_t FILT_GetSamplingRate(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* FILT_FILT_MAIN_H_ */
