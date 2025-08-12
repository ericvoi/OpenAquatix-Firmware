/*
 * mess_cargo.h
 *
 *  Created on: Jul 9, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_CARGO_H_
#define MESS_MESS_CARGO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_packet.h"
#include "mess_main.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds cargo bytes to the message along with error detection and the
 * length of the cargo
 * 
 * @param bit_msg Bit message to add the cargo to
 * @param msg message to take the cargo from
 * @param cfg Configurationd definign encoding
 * @return true if successful, false otherwise
 */
bool Cargo_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);

/**
 * @brief Extracts payload data from bit message into a structured message
 *
 * Converts the stream of bits in the bit message into bytes and copies them
 * to the message data field.
 *
 * @param bit_msg Pointer to the bit message containing the encoded data
 * @param msg Pointer to message structure where decoded data will be stored
 *
 * @return true if message extraction succeeds, false on extraction failure
 *
 * @pre Preamble must have been successfully received and decoded
 */
bool Cargo_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);

/**
 * @brief The non-FEC length of the cargo with coding applied
 * 
 * @param uncoded_len Uncoded (8-bit ASCII) length
 * @param coding_method How data is to be coded
 * @return Non-FEC cargo length with coding applied
 */
uint16_t Cargo_RawCodedLength(uint16_t uncoded_len, CodingInfo_t coding_method);

/**
 * @brief The non-FEC length of the cargo without any coding
 * 
 * @param coded_len Coded length of the cargo
 * @param coding_method How the length was coded
 * @return uint16_t 
 */
uint16_t Cargo_RawUncodedLength(uint16_t coded_len, CodingInfo_t coding_method);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_CARGO_H_ */
