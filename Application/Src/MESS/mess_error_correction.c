/*
 * mess_error_correction.c
 *
 *  Created on: Apr 30, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "main.h"
#include "mess_error_correction.h"
#include "mess_packet.h"
#include "number_utils.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint8_t message_buffer[PACKET_MAX_LENGTH_BYTES] = {0};

/* Private function prototypes -----------------------------------------------*/

static bool addHamming(BitMessage_t* bit_msg, bool is_preamble, uint16_t* bits_added);
static bool decodeHamming(BitMessage_t* bit_msg, bool is_preamble, bool* error_detected, bool* error_corrected);


static uint16_t calculateNumParityBits(const uint16_t num_bits);
static void clearBuffer(void);
static bool setBitInBuffer(bool bit, uint16_t position);
static bool getBitFromBuffer(uint16_t position, bool* bit);
static void copyBufferToMessage(BitMessage_t* bit_msg, uint16_t new_len);

/* Exported function definitions ---------------------------------------------*/

bool ErrorCorrection_AddCorrection(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  uint16_t bits_added = 0;
  clearBuffer();
  switch (cfg->ecc_method_preamble) {
    case NO_ECC:
      return true;
    case HAMMING_CODE:
      // add hamming code
      if (addHamming(bit_msg, true, &bits_added) == false) {
        return false;
      }
      break;
    default:
      return false;
  }

  switch (cfg->ecc_method_message) {
    case NO_ECC:
      return true;
    case HAMMING_CODE:
      if (addHamming(bit_msg, false, &bits_added) == false) {
        return false;
      }
      break;
    default:
      return false;
  }
  copyBufferToMessage(bit_msg, bits_added);
  return true;
}

bool ErrorCorrection_CheckCorrection(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble, bool* error_detected, bool* error_corrected)
{
  *error_detected = false;
  *error_corrected = false;
  if (is_preamble) {
    switch (cfg->ecc_method_preamble) {
      case NO_ECC:
        *error_detected = false;
        *error_corrected = false;
        return true;
      case HAMMING_CODE:
        return decodeHamming(bit_msg, true, error_detected, error_corrected);
      default:
        return false;
    }
  }
  else {
    switch (cfg->ecc_method_message) {
      case NO_ECC:
        *error_detected = false;
        *error_corrected = false;
        return true;
      case HAMMING_CODE:
        return decodeHamming(bit_msg, false, error_detected, error_corrected);
      default:
        return false;
    }
  }
}

uint16_t ErrorCorrection_GetLength(const uint16_t length, const ErrorCorrectionMethod_t method)
{
  switch (method) {
    case NO_ECC:
      return length;
    case HAMMING_CODE:
      return calculateNumParityBits(length) + length;
    default:
      return 0;
  }
}

/* Private function definitions ----------------------------------------------*/

bool addHamming(BitMessage_t* bit_msg, bool is_preamble, uint16_t* bits_added)
{
  uint16_t section_length = is_preamble ? PACKET_PREAMBLE_LENGTH_BITS :
      (bit_msg->final_length - PACKET_PREAMBLE_LENGTH_BITS);

  uint16_t start_raw_index = is_preamble ? 0 : PACKET_PREAMBLE_LENGTH_BITS;
  uint16_t start_ecc_index = is_preamble ? 0 :
      (calculateNumParityBits(PACKET_PREAMBLE_LENGTH_BITS) +
      PACKET_PREAMBLE_LENGTH_BITS);

  uint16_t parity_bits = calculateNumParityBits(section_length);

  uint16_t final_length = section_length + parity_bits;
  uint16_t message_bits_added = 0;
  // Add message bits to mesage
  for (uint16_t i = 0; i < final_length; i++) {
    if (NumberUtils_IsPowerOf2(i + 1) == true) {
      continue;
    }
    bool bit_to_add;
    if (Packet_GetBit(bit_msg, start_raw_index + message_bits_added++, &bit_to_add) == false) {
      return false;
    }
    if (setBitInBuffer(bit_to_add, start_ecc_index + i) == false) {
      return false;
    }
    if (message_bits_added >= section_length) break;
  }
  if (message_bits_added != section_length) {
    return false;
  }

  // Add parity bits at indices that are powers of 2 offset from the start index
  for (uint16_t p = 0; p < parity_bits; p++) {
    uint16_t parity_pos = (1 << p) - 1;
    bool parity = false;

    for (uint16_t i = parity_pos; i < final_length; i++) {
      if ((i + 1) & (1 << p)) {
        bool bit;
        if (getBitFromBuffer(i + start_ecc_index, &bit) == false) {
          return false;
        }
        parity ^= bit;
      }
    }

    if (setBitInBuffer(parity, parity_pos + start_ecc_index) == false) {
      return false;
    }
  }


  *bits_added += final_length;
  return true;
}

bool decodeHamming(BitMessage_t* bit_msg, bool is_preamble, bool* error_detected, bool* error_corrected)
{
  uint16_t start_raw_index = is_preamble ? 0 : PACKET_PREAMBLE_LENGTH_BITS;
  uint16_t start_ecc_index = is_preamble ? 0 :
      (calculateNumParityBits(PACKET_PREAMBLE_LENGTH_BITS) +
      PACKET_PREAMBLE_LENGTH_BITS);

  uint16_t raw_len = is_preamble ? PACKET_PREAMBLE_LENGTH_BITS :
      (bit_msg->non_preamble_length);
  uint16_t ecc_len = calculateNumParityBits(raw_len) + raw_len;

  uint16_t syndrome = 0;
  uint16_t parity_bits = 0;
  while ((1 << parity_bits) < ecc_len) {
    uint16_t p = parity_bits;
    bool parity = false;

    for (uint16_t i = 0; i < ecc_len; i++) {
      if ((i + 1) & (1 << p)) {
        bool bit;
        if (Packet_GetBit(bit_msg, i + start_ecc_index, &bit) == false) {
          return false;
        }
        parity ^= bit;
      }
    }
    if (parity != 0) {
      syndrome |= (1 << p);
    }

    parity_bits++;
  }

  if (syndrome != 0) {
    if (Packet_FlipBit(bit_msg, start_ecc_index + syndrome - 1) == false) {
      return false;
    }
    *error_corrected = true;
  }

  uint16_t decoded_pos = 0;
  for (uint16_t i = 0; i < ecc_len; i++) {
    if (NumberUtils_IsPowerOf2(i + 1) == false) {
      bool bit;
      if (Packet_GetBit(bit_msg, i + start_ecc_index, &bit) == false) {
        return false;
      }
      if (Packet_SetBit(bit_msg, decoded_pos + start_raw_index, bit) == false) {
        return false;
      }
      decoded_pos++;
      if (decoded_pos >= raw_len) break;
    }
  }

  bit_msg->combined_message_len = bit_msg->non_preamble_length +
      PACKET_PREAMBLE_LENGTH_BITS;

  return true;
}

uint16_t calculateNumParityBits(const uint16_t num_bits)
{
  uint16_t parity_bits = 0;

  while ((1 << parity_bits) < num_bits + parity_bits + 1) {
    parity_bits++;
  }
  return parity_bits;
}

void clearBuffer(void)
{
  memset(message_buffer, 0, sizeof(message_buffer) / sizeof(message_buffer[0]));
}

bool setBitInBuffer(bool bit, uint16_t position)
{
  if (position >= PACKET_MAX_LENGTH_BITS) {
    return false;
  }

  uint8_t byte_index = position / 8;
  uint8_t bit_position = position % 8;

  if (bit == true) {
    message_buffer[byte_index] |= (1 << (7 - bit_position));
  } else {
    message_buffer[byte_index] &= ~(1 << (7 - bit_position));
  }
  return true;
}

bool getBitFromBuffer(uint16_t position, bool* bit)
{
  if (position >= PACKET_MAX_LENGTH_BITS) {
    return false;
  }

  uint16_t byte_index = position / 8;
  uint8_t bit_position = position % 8;

  *bit = (message_buffer[byte_index] & (1 << (7 - bit_position))) != 0;

  return true;
}

void copyBufferToMessage(BitMessage_t* bit_msg, uint16_t new_len)
{
  memcpy(bit_msg->data, message_buffer, sizeof(message_buffer) / sizeof(message_buffer[0]));
  bit_msg->bit_count = new_len;
  bit_msg->final_length = new_len;
}
