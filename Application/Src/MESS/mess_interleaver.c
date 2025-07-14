/*
 * mess_interleaver.c
 *
 *  Created on: Jun 1, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_interleaver.h"
#include "mess_packet.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

// First 50 prime numbers
const uint16_t primes[50] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151,
    157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229
};

static const uint16_t num_primes = sizeof(primes) / sizeof(primes[0]);

/* Private function prototypes -----------------------------------------------*/

// These functions directly modify the input bit message. Both functions are
// nearly identical besides changing the original/destination indices
static bool interleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg);
static bool deinterleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg);
static uint16_t findInterleavingDepth(uint16_t length);


/* Exported function definitions ---------------------------------------------*/

bool Interleaver_Apply(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (cfg->use_interleaver == false) {
    return true;
  }
  // First interleave the preamble separately following the JANUS standard
  BitMessage_t buffer_bit_msg;
  buffer_bit_msg.bit_count = bit_msg->bit_count;
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
  buffer_bit_msg.bit_count = bit_msg->bit_count;
  SectionInfo_t section_info = is_preamble ? bit_msg->preamble : bit_msg->cargo;

  return deinterleave(section_info.ecc_start_index, section_info.ecc_len,
      bit_msg, &buffer_bit_msg);
}

/* Private function definitions ----------------------------------------------*/

bool interleave(uint16_t start_index, uint16_t length,
    BitMessage_t* input_bit_msg, BitMessage_t* buffer_bit_msg)
{
  // If the length and the interleaver depth have a common denominator other
  // than 1, the interleaver will result in duplicate entries and lost data
  uint16_t interleaver_depth = findInterleavingDepth(length);
  if ((length % interleaver_depth) == 0) {
    return false;
  }

  for (uint16_t i = 0; i < length; i++) {
    // Values are 32 bits to handle intermediate results
    uint32_t original_index = start_index + i;
    uint32_t new_index = (i * interleaver_depth) % length + start_index;
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
  // If the length and the interleaver depth have a common denominator other
  // than 1, the interleaver will result in duplicate entries and lost data
  uint16_t interleaver_depth = findInterleavingDepth(length);
  if ((length % interleaver_depth) == 0) {
    return false;
  }

  for (uint16_t i = 0; i < length; i++) {
    // Values are 32 bits to handle intermediate results
    uint32_t original_index = (i * interleaver_depth) % length + start_index;
    uint32_t new_index = start_index + i;
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

/**
 * Following the JANUS standard (ANEP-87) The interleaver depth (D) must meet
 * the following two criteria:
 * 1. D^2 > L where L is the length of the section with ECC
 * 2. D is not a factor of L
 */
uint16_t findInterleavingDepth(uint16_t length)
{
  for (uint16_t i = 0; i < num_primes; i++) {
    uint16_t candidate_prime = primes[i];
    if (candidate_prime * candidate_prime <= length) {
      continue;
    }

    if (length % candidate_prime != 0) {
      return candidate_prime;
    }
  }

  // No message should ever get here, but if it somehow does, remove condition 1
  for (uint16_t i = num_primes - 1; i > 0; i++) {
    uint16_t candidate_prime = primes[i];
    if (length % candidate_prime != 0) {
      return candidate_prime;
    }
  }
  // Absolutely no messages should ever get here
  return 1; // no interleaving
}
