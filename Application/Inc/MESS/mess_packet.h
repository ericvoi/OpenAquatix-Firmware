/*
 * mess_packet.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_PACKET_H_
#define MESS_MESS_PACKET_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_main.h"
#include "mess_dsp_config.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/


typedef struct {
  uint8_t data[PACKET_MAX_LENGTH_BYTES];
  uint16_t bit_count;
  uint8_t sender_id;
  uint16_t data_len_bits;
  MessageData_t contents_data_type;
  uint16_t preamble_length_ecc;
  uint16_t final_length; // includes ecc
  uint16_t non_preamble_length_ecc;
  uint16_t non_preamble_length;
  uint16_t combined_message_len; // not including ecc
  float normalized_vitrebi_error_metric; // Only set when the ecc method uses convoltuional codes
  bool stationary_flag;
  bool preamble_received; // Set when first preamble number of bits received and decoded
  bool fully_received;    // Set when message bit count > final bit count
  bool added_to_queue;    // Set when message decoded and "done with"
  bool error_preamble;
  bool error_message;
  bool error_entire_message;
  bool corrected_error_preamble;
  bool corrected_error_message;
} BitMessage_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Prepares a message for transmission by encoding it into a bit packet
 *
 * This function performs the complete packet preparation sequence:
 * 1. Initializes the bit packet
 * 2. Adds preamble (skipped for EVAL type messages)
 * 3. Adds message payload
 * 4. Applies error detection coding (skipped for EVAL type messages)
 *
 * @param msg Pointer to the message to be transmitted
 * @param bit_msg Pointer to the bit message structure to be filled
 * @param cfg Configuration data for the message
 *
 * @return true if preparation succeeded, false on any failure
 */
bool Packet_PrepareTx(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg);

/**
 * @brief Initializes a bit message structure for receiving incoming data
 *
 * @param bit_msg Pointer to the bit message structure to initialize
 *
 * @return true if initialization succeeded
 */
bool Packet_PrepareRx(BitMessage_t* bit_msg);

/**
 * @brief Adds a single bit to a bit message
 *
 * Appends a bit to the end of the bit message, handling the necessary
 * byte and bit indexing operations.
 *
 * @param bit_msg Pointer to the bit message structure
 * @param bit The bit value to add (true=1, false=0)
 *
 * @return true if successful, false if packet is already at maximum capacity
 */
bool Packet_AddBit(BitMessage_t* bit_msg, bool bit);

/**
 * @brief Retrieves a bit value from a specific position in a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param position Zero-based index of the bit to retrieve
 * @param bit Pointer where the retrieved bit will be stored
 *
 * @return true if successful, false if position is out of bounds
 */
bool Packet_GetBit(BitMessage_t* bit_msg, uint16_t position, bool* bit);

/**
 * @brief Extracts an arbitrary-length chunk of bits (up to 8) from a bit message
 *
 * This function handles misaligned bit access across byte boundaries.
 *
 * @param bit_msg Pointer to the bit message structure
 * @param start_position Pointer to the starting bit position (will be updated)
 * @param chunk_length Number of bits to extract (must be ≤ 8)
 * @param ret Pointer where the extracted bit chunk will be stored
 *
 * @return true if successful, false if requested bits exceed message bounds
 */
bool Packet_Get8BitChunk(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t chunk_length, uint8_t* ret);

/**
 * @brief Adds an 8-bit value to a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param data The 8-bit value to add
 *
 * @return true if successful, false if packet would exceed maximum size
 */
bool Packet_Add8(BitMessage_t* bit_msg, uint8_t data);

/**
 * @brief Adds a 16-bit value to a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param data The 16-bit value to add
 *
 * @return true if successful, false if packet would exceed maximum size
 */
bool Packet_Add16(BitMessage_t* bit_msg, uint16_t data);

/**
 * @brief Adds a 32-bit value to a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param data The 32-bit value to add
 *
 * @return true if successful, false if packet would exceed maximum size
 */
bool Packet_Add32(BitMessage_t* bit_msg, uint32_t data);

/**
 * @brief Extracts an 8-bit value from a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param start_position Pointer to the starting bit position (will be updated)
 * @param data Pointer where the extracted value will be stored
 *
 * @return true if successful, false if requested bits exceed message bounds
 */
bool Packet_Get8(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t* data);

/**
 * @brief Extracts a 16-bit value from a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param start_position Pointer to the starting bit position (will be updated)
 * @param data Pointer where the extracted value will be stored
 *
 * @return true if successful, false if requested bits exceed message bounds
 */
bool Packet_Get16(BitMessage_t* bit_msg, uint16_t* start_position, uint16_t* data);

/**
 * @brief Extracts a 32-bit value from a bit message
 *
 * @param bit_msg Pointer to the bit message structure
 * @param start_position Pointer to the starting bit position (will be updated)
 * @param data Pointer where the extracted value will be stored
 *
 * @return true if successful, false if requested bits exceed message bounds
 */
bool Packet_Get32(BitMessage_t* bit_msg, uint16_t* start_position, uint32_t* data);

/**
 * @brief Flips bit at selected position
 *
 * @param bit_msg Pointer to the bit message structure
 * @param bit_index Index where the bit should be flipped
 *
 * @return true if successful, false if invalid inputs
 */
bool Packet_FlipBit(BitMessage_t* bit_msg, uint16_t bit_index);

/**
 * @brief Sets bit at a certain position
 *
 * @param bit_msg Pointer to the bit message structure
 * @param bit_index Index where the bit should be set
 * @param bit Value of the bit
 *
 * @return true if successful, false otherwise
 */
bool Packet_SetBit(BitMessage_t* bit_msg, uint16_t bit_index, bool bit);

/**
 * @brief Compares the data in 2 bit messages
 *
 * @param msg1 Pointer to the first message to compare
 * @param msg2 Pointer to the second message to compare
 * @param identical Returned value indicating if all bits identical
 *
 * @return true if successful and false otherwise
 */
bool Packet_Compare(const BitMessage_t* msg1, const BitMessage_t* msg2, bool* identical);

/**
 * @brief Calculates the minimum power-of-2 packet size needed for a given payload
 *
 * @param str_len The length of data to accommodate
 *
 * @return The minimum packet size (always a power of 2)
 */
uint16_t Packet_MinimumSize(uint16_t str_len);

/**
 * @brief Registers modem parameters with the parameter subsystem for HMI access
 *
 * Registers:
 * - The modem identifier
 * - The stationary flag (indicating if the modem is in a fixed position)
 *
 * @return true if all parameters were registered successfully
 */
bool Packet_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_PACKET_H_ */
