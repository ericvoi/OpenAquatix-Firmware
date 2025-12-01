/*
 * goertzel.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMMON_UTILS_GOERTZEL_H_
#define COMMON_UTILS_GOERTZEL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  uint16_t buf_len;
  uint16_t data_len;
  uint16_t start_pos;
  uint32_t* f;
  float* e_f;
  float* window;
  float energy_normalization;
  uint16_t window_size;
} GoertzelInfo_t;

typedef struct {
  uint32_t f;
  float e_f;
  float q[3];
  float coeff;
  float normalization_factor;
  uint16_t window_length;
  uint16_t calls_before_reset;
} SlidingGoertzelInfo_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Calculates goertzel on a single frequency
 * 
 * @param goertzel_info Contains input and output info for goertzel calculation
 */
void goertzel_1(GoertzelInfo_t* goertzel_info);

/**
 * @brief Calculates goertzel on 2 frequencies together
 * 
 * @param goertzel_info Contains input and output info for goertzel calculation
 */
void goertzel_2(GoertzelInfo_t* goertzel_info);

/**
 * @brief Calculates goertzel on 6 frequencies at once
 * 
 * @param goertzel_info Contains input and output info for goertzel calculation
 */
void goertzel_6(GoertzelInfo_t* goertzel_info);

/**
 * @brief Initializes a sliding goertzel window
 * 
 * @param goertzel_info Structure populated with initialized goertzel information
 * @param f Frequency to initialize to
 * @param window_length Number of samples in the window
 */
void goertzel_SlidingInit(SlidingGoertzelInfo_t* goertzel_info, uint32_t f, uint16_t window_length);

/**
 * @brief Performs a sliding goertzel filter on the incoming data
 * 
 * @param goertzel_info Struct containing previous results
 * @param start_index Starting index in the ADC buffer to use
 * @param samples The number of ADC samples over which to perform the filter
 * @param buf_len The length of the entire ADC buffer (must be a power of 2)
 */
void goertzel_SlidingPerform(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t samples, uint16_t buf_len);

void goertzel_SlidingReset(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t buf_len);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_GOERTZEL_H_ */
