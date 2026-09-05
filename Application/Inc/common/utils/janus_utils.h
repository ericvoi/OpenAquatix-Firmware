/*
 * janus_utils.h
 *
 *  Created on: Jul 16, 2026
 *      Author: ericv
 * 
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMMON_UTILS_JANUS_UTILS_H_
#define COMMON_UTILS_JANUS_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define MAXIMUM_RESERVATION_TIME_MS   (632640) // Maximum reservation time in ms as specified by JANUS
#define JANUS_RESERVATION_BITS        7
#define JANUS_N_E                     2
#define JANUS_N_X                     5

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Encodes a time into a 7-bit JANUS reservation
 * 
 * @param time_ms Entire cargo duration in ms
 * @return uint16_t Reservation code
 */
uint16_t JanusUtil_EncodeResTime(uint32_t time_ms);

/**
 * @brief decodes a 7-bit reservation code
 * 
 * @param coded_value 7-bit reservation code
 * @return uint32_t reservation duration in ms
 */
uint32_t JanusUtil_DecodeResTime(uint16_t coded_value);

uint16_t JanusUtil_EncodeLen(uint16_t n_bits,  uint8_t n_e, uint8_t n_x);
uint16_t JanusUtil_DecodeLen(uint16_t encoded, uint8_t n_e, uint8_t n_x);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_JANUS_UTILS_H_ */
