/*
 * mess_preamble.h
 *
 *  Created on: Jul 6, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_PREAMBLE_H_
#define MESS_MESS_PREAMBLE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_main.h"
#include "mess_packet.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds raw preamble information bits to a message before transmission
 * 
 * @param bit_msg Bit message to add the preamble bits to 
 * @param msg Message defining what is inside the preamble
 * @param cfg Configuration values defining error detection method
 * @return true if successfully added preamble, false otherwise
 */
bool Preamble_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);

/**
 * @brief Extracts bits from preamble and adds meaning to them according to the
 * preamble recipe
 * 
 * @param bit_msg Bit message to take the preamble bits from (must be unencoded)
 * @param msg Message to add the preamble information to
 * @param cfg Configuration values for decoding
 * @return true if successfully decoded preamble, false otherwise.
 */
bool Preamble_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);

/**
 * @brief Updates the number of bits in the bit message preamble
 * 
 * @param bit_msg Bit message to update the preamble of
 * @param cfg Configuration values that define error detection and correction
 * @return true if successful, false otherwise
 */
bool Preamble_UpdateNumBits(BitMessage_t* bit_msg, const DspConfig_t* cfg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_PREAMBLE_H_ */
