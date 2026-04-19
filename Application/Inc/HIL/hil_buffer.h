/*
 * hil_buffer.h
 *
 *  Created on: Apr 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef HIL_HIL_BUFFER_H_
#define HIL_HIL_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds data to the HIL ring buffer
 * 
 * @param src Source array to take the data from
 * @param src_len Length of the source array
 * 
 * @note This does not handle wrap around so in case of wrap around, the caller
 * must handle this
 */
void HilBuf_AddData(volatile const uint16_t* src, uint16_t src_len);

/**
 * @brief Gets data from the HIL ring buffer. Adds a mid-value to fill in gaps
 * 
 * @param dst Destination array (modified)
 * @param samples The number of samples to take
 * @param mid_value In case there are not enough samples in the ring buffer, 
 * use this value 
 */
void HilBuf_GetData(volatile uint16_t* dst, uint16_t samples, uint16_t mid_value);

/**
 * @brief Checks the HIL stream endpoint for any new rx packets. Drains the rx
 * stream of any packets
 */
void HilBuf_ReadRxPackets(void);

/**
 * @brief Checks buffer to see if a new tx packet can be sent. If so, sends as
 * many as possible
 */
void HilBuf_SendTxPackets(void);

/**
 * @brief Resets the HIL ring buffer by setting all entries to 0 and resetting
 * head/tail
 */
void HilBuf_Reset(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_BUFFER_H_ */
