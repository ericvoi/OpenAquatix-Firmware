/*
 * mess_interleaver.c
 *
 *  Created on: Jun 1, 2025
 *      Author: ericv
 */

/*
 * Notes on interleaver:
 * 1. The interleaver can only be applied to a stream of bits if the length is
 * not a multiple of the interleaver depth
 * 2. Following the JANUS standard (ANEP-87,) the preamble and the message
 * cargo is interleaved separately
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_interleaver.h"
#include "mess_packet.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define INTERLEAVER_DEPTH   13 // Same as JANUS (ANEP-87)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

// These functions directly modify the input bit message. Both functions are
// nearly identical besides changing the original/destination indices
bool interleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg);
bool deinterleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg);


/* Exported function definitions ---------------------------------------------*/

bool Interleaver_Apply(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (cfg->use_interleaver == false) {
    return true;
  }
  // First interleave the preamble separately
  BitMessage_t buffer_bit_msg;
  if (interleave(bit_msg->preamble.ecc_start_index, bit_msg->preamble.ecc_len,
                 bit_msg, &buffer_bit_msg) == false) {
    return false;
  }

  // Then interleave the message cargo separately
  if (interleave(bit_msg->cargo.ecc_start_index, bit_msg->cargo.ecc_len,
                 bit_msg, &buffer_bit_msg) == false) {
    return false;
  }
  return true;
}

bool Interleaver_Undo(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble)
{
  if (cfg->use_interleaver == false) {
    return true;
  }
  BitMessage_t buffer_bit_msg;
  SectionInfo_t section_info = is_preamble ? bit_msg->preamble : bit_msg->cargo;

  return deinterleave(section_info.ecc_start_index, section_info.ecc_len,
      bit_msg, &buffer_bit_msg);
}

/* Private function definitions ----------------------------------------------*/

bool interleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg)
{
  if ((length % INTERLEAVER_DEPTH) == 0) {
    return false;
  }

  for (uint16_t i = 0; i < length; i++) {
    // Values are 32 bits to handle intermediate results
    uint32_t original_index = start_index + i;
    uint32_t new_index = (i * INTERLEAVER_DEPTH) % length + start_index;
    bool bit;
    if (Packet_GetBit(input_bit_msg, (uint16_t) original_index, &bit) == false) {
      return false;
    }
    if (Packet_SetBit(buffer_bit_msg, (uint16_t) new_index, bit) == false) {
      return false;
    }
  }

  if (Packet_Copy(buffer_bit_msg, input_bit_msg, start_index, length) == false) {
    return false;
  }
  return true;
}

bool deinterleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg)
{
  if ((length % INTERLEAVER_DEPTH) == 0) {
    return false;
  }

  for (uint16_t i = 0; i < length; i++) {
    // Values are 32 bits to handle intermediate results
    uint32_t new_index = start_index + i;
    uint32_t original_index = (i * INTERLEAVER_DEPTH) % length + start_index;
    bool bit;
    if (Packet_GetBit(input_bit_msg, (uint16_t) original_index, &bit) == false) {
      return false;
    }
    if (Packet_SetBit(buffer_bit_msg, (uint16_t) new_index, bit) == false) {
      return false;
    }
  }

  if (Packet_Copy(buffer_bit_msg, input_bit_msg, start_index, length) == false) {
    return false;
  }

  return true;
}
