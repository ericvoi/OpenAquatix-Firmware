/*
 * mess_interleaver.h
 *
 *  Created on: Jun 1, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_INTERLEAVER_H_
#define MESS_MESS_INTERLEAVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Interleaves a message following the JANUS standard
 * 
 * @param bit_msg Message to be interleaved (modified)
 * @param cfg Configuration values (decides whether interleaving should occur)
 * @return true if successful false otherwise
 * 
 * @see Interleaver_Undo()
 */
void Interleaver_Apply(BitMessage_t* bit_msg, const DspConfig_t* cfg);

/**
 * @brief Deinterleaves a message following the JANUS standard
 * 
 * @param bit_msg Message with interleaving applied (modified)
 * @param cfg Configuration values (decides whether interleaving should occur)
 * @param is_preamble Which part of the message to deinterleave
 * 
 * @see Interleaver_Apply()
 */
void Interleaver_Undo(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_INTERLEAVER_H_ */
