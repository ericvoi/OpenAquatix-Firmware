/*
 * mess_error_correction.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_error_detection.h"
#include "mess_packet.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

// x^8 + x^2 + x^1 + 1
#define CRC_8_POLYNOMIAL  0x07U 
// x^16 + x^15 + x^2 + 1
#define CRC_16_POLYNOMIAL 0x8005U
// x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
#define CRC_32_POLYNOMIAL 0x04C11DB7U 

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void calculateCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* crc);
void calculateCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* crc);
void calculateCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* crc);
void calculateChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* checksum);
void calculateChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* checksum);
void calculateChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* checksum);

void checkCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
void checkCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
void checkCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
void checkChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
void checkChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
void checkChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);

/* Exported function definitions ---------------------------------------------*/

void ErrorDetection_AddDetection(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble)
{
  uint16_t start_bit;
  uint16_t end_bit;
  ErrorDetectionMethod_t method;
  if (is_preamble == true) {
    start_bit = bit_msg->preamble.raw_start_index;
    end_bit = start_bit + bit_msg->preamble.raw_len - 1;
    method = cfg->preamble_validation;
  }
  else {
    start_bit = bit_msg->cargo.raw_start_index;
    end_bit = start_bit + bit_msg->cargo.raw_len - 1;
    method = cfg->cargo_validation;
  }

  uint16_t len;
  ErrorDetection_CheckLength(&len, method);
  end_bit -= len;
  switch (method) {
    case CRC_8:
      uint8_t crc_8;
      calculateCrc8(bit_msg, start_bit, end_bit, &crc_8);
      RETURN_IF_ERROR_PRESENT(Packet_Add8(bit_msg, crc_8));
      break;
    case CRC_16:
      uint16_t crc_16;
      calculateCrc16(bit_msg, start_bit, end_bit, &crc_16);
      RETURN_IF_ERROR_PRESENT(Packet_Add16(bit_msg, crc_16));
      break;
    case CRC_32:
      uint32_t crc_32;
      calculateCrc32(bit_msg, start_bit, end_bit, &crc_32);
      RETURN_IF_ERROR_PRESENT(Packet_Add32(bit_msg, crc_32));
      break;
    case CHECKSUM_8:
      uint8_t checksum_8;
      RETURN_IF_ERROR_PRESENT(calculateChecksum8(bit_msg, start_bit, end_bit, &checksum_8));
      RETURN_IF_ERROR_PRESENT(Packet_Add8(bit_msg, checksum_8));
      break;
    case CHECKSUM_16:
      uint16_t checksum_16;
      RETURN_IF_ERROR_PRESENT(calculateChecksum16(bit_msg, start_bit, end_bit, &checksum_16));
      RETURN_IF_ERROR_PRESENT(Packet_Add16(bit_msg, checksum_16));
      break;
    case CHECKSUM_32:
      uint32_t checksum_32;
      RETURN_IF_ERROR_PRESENT(calculateChecksum32(bit_msg, start_bit, end_bit, &checksum_32));
      RETURN_IF_ERROR_PRESENT(Packet_Add32(bit_msg, checksum_32));
      break;
    case NO_ERROR_DETECTION:
      return;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void ErrorDetection_CheckDetection(BitMessage_t* bit_msg, bool* error, const DspConfig_t* cfg, bool is_preamble)
{
  RETURN_IF_ERROR_PRESENT();
  uint16_t start_bit;
  uint16_t end_bit;
  ErrorDetectionMethod_t method;
  if (is_preamble == true) {
    start_bit = bit_msg->preamble.raw_start_index;
    end_bit = start_bit + bit_msg->preamble.raw_len - 1;
    method = cfg->preamble_validation;
  }
  else {
    start_bit = bit_msg->cargo.raw_start_index;
    end_bit = start_bit + bit_msg->cargo.raw_len - 1;
    method = cfg->cargo_validation;
  }
  uint16_t len;
  ErrorDetection_CheckLength(&len, method);
  // Received messages have the error detection bits added to the length
  end_bit -= len;
  switch (method) {
    case CRC_8:
      RETURN_IF_ERROR_PRESENT(checkCrc8(bit_msg, start_bit, end_bit, error));
      break;
    case CRC_16:
      RETURN_IF_ERROR_PRESENT(checkCrc16(bit_msg, start_bit, end_bit, error));
      break;
    case CRC_32:
      RETURN_IF_ERROR_PRESENT(checkCrc32(bit_msg, start_bit, end_bit, error));
      break;
    case CHECKSUM_8:
      RETURN_IF_ERROR_PRESENT(checkChecksum8(bit_msg, start_bit, end_bit, error));
      break;
    case CHECKSUM_16:
      RETURN_IF_ERROR_PRESENT(checkChecksum16(bit_msg, start_bit, end_bit, error));
      break;
    case CHECKSUM_32:
      RETURN_IF_ERROR_PRESENT(checkChecksum32(bit_msg, start_bit, end_bit, error));
      break;
    case NO_ERROR_DETECTION:
      return;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void ErrorDetection_CheckLength(uint16_t* length, ErrorDetectionMethod_t method)
{
  switch (method) {
    case CRC_8:
    case CHECKSUM_8:
      *length = 8;
      return;
    case CRC_16:
    case CHECKSUM_16:
      *length = 16;
      return;
    case CRC_32:
    case CHECKSUM_32:
      *length = 32;
      return;
    case NO_ERROR_DETECTION:
      *length = 0;
      return;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void ErrorDetection_RegisterParams(void)
{
  return;
}

/* Private function definitions ----------------------------------------------*/

void calculateCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* crc)
{
  uint8_t polynomial = CRC_8_POLYNOMIAL;
  *crc = 0;

  uint16_t current_bit = start_bit;
  // Handle bits until we reach byte alignment or end
  while (current_bit <= end_bit && (current_bit % 8 != 0)) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
    
    // Bit-by-bit processing for unaligned bits
    uint8_t bit_out = (*crc >> 7) & 1;
    *crc = (*crc << 1) & 0xFF;
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }

  while (current_bit <= end_bit && (end_bit - current_bit + 1) >= 8) {
    uint16_t byte_index = current_bit / 8;
      
    *crc ^= bit_msg->data[byte_index];
    for (int j = 0; j < 8; j++) {
      if (*crc & 0x80) {
        *crc = (*crc << 1) ^ polynomial;
      } 
      else {
        *crc = *crc << 1;
      }
    }
    current_bit += 8;
  }

  // Handle remaining bits at the end
  while (current_bit <= end_bit) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
    
    // Bit-by-bit processing for remaining bits
    uint8_t bit_out = (*crc >> 7) & 1;
    *crc = (*crc << 1) & 0xFF;
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }
}

void calculateCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* crc)
{
  uint16_t polynomial = CRC_16_POLYNOMIAL;
  *crc = 0xFFFF;

  uint16_t current_bit = start_bit;

  // Handle bits until we reach byte alignment or end
  while (current_bit <= end_bit && (current_bit % 8 != 0)) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
    
    // Bit-by-bit processing for unaligned bits
    uint8_t bit_out = (*crc >> 15) & 1;
    *crc = (*crc << 1) & 0xFFFF;
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }

  // Process full bytes using efficient byte-wise method
  while (current_bit <= end_bit && (end_bit - current_bit + 1) >= 8) {
    uint16_t byte_index = current_bit / 8;
      
    // XOR input byte with high byte of CRC
    *crc ^= (uint16_t) (bit_msg->data[byte_index]) << 8;
      
    for (int j = 0; j < 8; j++) {
      if (*crc & 0x8000U) {
        *crc = (*crc << 1) ^ polynomial;
      } 
      else {
        *crc = *crc << 1;
      }
    }
    current_bit += 8;
  }

    // Handle remaining bits at the end
  while (current_bit <= end_bit) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
        
    // Bit-by-bit processing for remaining bits
    uint8_t bit_out = (*crc >> 15) & 1;
    *crc = (*crc << 1) & 0xFFFF;
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }
}

void calculateCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* crc)
{
  uint32_t polynomial = CRC_32_POLYNOMIAL;
  *crc = 0xFFFFFFFF;

  uint16_t current_bit = start_bit;

  // Handle bits until we reach byte alignment or end
  while (current_bit <= end_bit && (current_bit % 8 != 0)) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
    
    // Bit-by-bit processing for unaligned bits
    uint8_t bit_out = (*crc >> 31) & 1;
    *crc = (*crc << 1);
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }

  // Process full bytes using efficient byte-wise method
  while (current_bit <= end_bit && (end_bit - current_bit + 1) >= 8) {
    uint16_t byte_index = current_bit / 8;
    
    // XOR input byte with high byte of CRC
    *crc ^= (uint32_t)(bit_msg->data[byte_index]) << 24;
    
    for (int j = 0; j < 8; j++) {
      if (*crc & 0x80000000U) {
        *crc = (*crc << 1) ^ polynomial;
      } else {
        *crc = *crc << 1;
      }
    }
    current_bit += 8;
  }

  // Handle remaining bits at the end
  while (current_bit <= end_bit) {
    uint16_t byte_index = current_bit / 8;
    uint16_t bit_offset = current_bit % 8;
    uint8_t input_bit = (bit_msg->data[byte_index] >> (7 - bit_offset)) & 1;
    
    // Bit-by-bit processing for remaining bits
    uint8_t bit_out = (*crc >> 31) & 1;
    *crc = (*crc << 1);
    if (bit_out ^ input_bit) {
      *crc ^= polynomial;
    }
    current_bit++;
  }
    
  *crc = ~*crc;
}

void calculateChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* checksum)
{
  uint16_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 8) {
    uint8_t chunk;
    if (Packet_Get8(bit_msg, &current_bit, &chunk) == false)
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
    intermediate += chunk;
    if (((intermediate >> 8) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint8_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false) 
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

    chunk |= bit << (7 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 8) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFF;
  }
  *checksum = intermediate & 0xFF;
}

void calculateChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* checksum)
{
  uint32_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 16) {
    uint16_t chunk;
    RETURN_IF_ERROR_PRESENT(Packet_Get16(bit_msg, &current_bit, &chunk));
    intermediate += chunk;
    if (((intermediate >> 16) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFFFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint16_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false)
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
    
    chunk |= bit << (15 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 16) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFFFF;
  }
  *checksum = intermediate & 0xFFFF;
}

void calculateChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* checksum)
{
  uint64_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 32) {
    uint32_t chunk;
    RETURN_IF_ERROR_PRESENT(Packet_Get32(bit_msg, &current_bit, &chunk));
    intermediate += chunk;
    if (((intermediate >> 8) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint32_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false)
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
    
    chunk |= bit << (31 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 32) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFFFFFFFF;
  }
  *checksum = intermediate & 0xFFFFFFFF;
}



void checkCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint8_t theoretical_crc;
  calculateCrc8(bit_msg, start_bit, end_bit, &theoretical_crc);
  uint8_t actual_crc;
  end_bit++;
  if (Packet_Get8(bit_msg, &end_bit, &actual_crc) == false)
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_crc != theoretical_crc;
}

void checkCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint16_t theoretical_crc;
  calculateCrc16(bit_msg, start_bit, end_bit, &theoretical_crc);
  uint16_t actual_crc;
  end_bit++;
  if (Packet_Get16(bit_msg, &end_bit, &actual_crc) == false) 
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_crc != theoretical_crc;
}

void checkCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint32_t theoretical_crc;
  calculateCrc32(bit_msg, start_bit, end_bit, &theoretical_crc);
  uint32_t actual_crc;
  end_bit++;
  if (Packet_Get32(bit_msg, &end_bit, &actual_crc) == false) 
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_crc != theoretical_crc;
}

void checkChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint8_t theoretical_checksum;
  RETURN_IF_ERROR_PRESENT(calculateChecksum8(bit_msg, start_bit, end_bit, &theoretical_checksum));
  uint8_t actual_checksum;
  end_bit++;
  if (Packet_Get8(bit_msg, &end_bit, &actual_checksum) == false) 
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_checksum != theoretical_checksum;
}

void checkChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint16_t theoretical_checksum;
  RETURN_IF_ERROR_PRESENT(calculateChecksum16(bit_msg, start_bit, end_bit, &theoretical_checksum));
  uint16_t actual_checksum;
  end_bit++;
  if (Packet_Get16(bit_msg, &end_bit, &actual_checksum) == false)
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_checksum != theoretical_checksum;
}

void checkChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint32_t theoretical_checksum;
  RETURN_IF_ERROR_PRESENT(calculateChecksum32(bit_msg, start_bit, end_bit, &theoretical_checksum));
  uint32_t actual_checksum;
  end_bit++;
  if (Packet_Get32(bit_msg, &end_bit, &actual_checksum) == false)
    REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);

  *error = actual_checksum != theoretical_checksum;
}
