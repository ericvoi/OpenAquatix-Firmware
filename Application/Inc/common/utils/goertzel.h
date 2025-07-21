/*
 * goertzel.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
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

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_GOERTZEL_H_ */
