/*
 * mess_error_correction.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include <mess_error_detection.h>
#include "mess_packet.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
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

bool calculateCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* crc);
bool calculateCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* crc);
bool calculateCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* crc);
bool calculateChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* checksum);
bool calculateChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* checksum);
bool calculateChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* checksum);

bool checkCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
bool checkCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
bool checkCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
bool checkChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
bool checkChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);
bool checkChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error);

/* Exported function definitions ---------------------------------------------*/

bool ErrorDetection_AddDetection(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble)
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
  if (ErrorDetection_CheckLength(&len, method) == false) {
    return false;
  }
  end_bit -= len;
  switch (method) {
    case CRC_8:
      uint8_t crc_8;
      if (calculateCrc8(bit_msg, start_bit, end_bit, &crc_8) == false) {
        return false;
      }
      return Packet_Add8(bit_msg, crc_8);
    case CRC_16:
      uint16_t crc_16;
      if (calculateCrc16(bit_msg, start_bit, end_bit, &crc_16) == false) {
        return false;
      }
      return Packet_Add16(bit_msg, crc_16);
    case CRC_32:
      uint32_t crc_32;
      if (calculateCrc32(bit_msg, start_bit, end_bit, &crc_32) == false) {
        return false;
      }
      return Packet_Add32(bit_msg, crc_32);
    case CHECKSUM_8:
      uint8_t checksum_8;
      if (calculateChecksum8(bit_msg, start_bit, end_bit, &checksum_8) == false) {
        return false;
      }
      return Packet_Add8(bit_msg, checksum_8);
    case CHECKSUM_16:
      uint16_t checksum_16;
      if (calculateChecksum16(bit_msg, start_bit, end_bit, &checksum_16) == false) {
        return false;
      }
      return Packet_Add16(bit_msg, checksum_16);
    case CHECKSUM_32:
      uint32_t checksum_32;
      if (calculateChecksum32(bit_msg, start_bit, end_bit, &checksum_32) == false) {
        return false;
      }
      return Packet_Add32(bit_msg, checksum_32);
    case NO_ERROR_DETECTION:
      return true;
    default:
      return false;
  }
}

bool ErrorDetection_CheckDetection(BitMessage_t* bit_msg, bool* error, const DspConfig_t* cfg, bool is_preamble)
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
  if (ErrorDetection_CheckLength(&len, method) == false) {
    return false;
  }
  // Received messages have the error detection bits added to the length
  end_bit -= len;
  switch (method) {
    case CRC_8:
      return checkCrc8(bit_msg, start_bit, end_bit, error);
    case CRC_16:
      return checkCrc16(bit_msg, start_bit, end_bit, error);
    case CRC_32:
      return checkCrc32(bit_msg, start_bit, end_bit, error);
    case CHECKSUM_8:
      return checkChecksum8(bit_msg, start_bit, end_bit, error);
    case CHECKSUM_16:
      return checkChecksum16(bit_msg, start_bit, end_bit, error);
    case CHECKSUM_32:
      return checkChecksum32(bit_msg, start_bit, end_bit, error);
    case NO_ERROR_DETECTION:
      return true;
    default:
      return false;
  }
}

bool ErrorDetection_CheckLength(uint16_t* length, ErrorDetectionMethod_t method)
{
  switch (method) {
    case CRC_8:
    case CHECKSUM_8:
      *length = 8;
      return true;
    case CRC_16:
    case CHECKSUM_16:
      *length = 16;
      return true;
    case CRC_32:
    case CHECKSUM_32:
      *length = 32;
      return true;
    case NO_ERROR_DETECTION:
      *length = 0;
      return true;
    default:
      return false;
  }
}

bool ErrorDetection_RegisterParams(void)
{
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool calculateCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

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

  return true;
}

bool calculateCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

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
    
  return true;
}

bool calculateCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

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
  return true;
}

bool calculateChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint8_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  uint16_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 8) {
    uint8_t chunk;
    if (Packet_Get8(bit_msg, &current_bit, &chunk) == false) {
      return false;
    }
    intermediate += chunk;
    if (((intermediate >> 8) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint8_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false) {
      return false;
    }
    chunk |= bit << (7 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 8) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFF;
  }
  *checksum = intermediate & 0xFF;
  return true;
}

bool calculateChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint16_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  uint32_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 16) {
    uint16_t chunk;
    if (Packet_Get16(bit_msg, &current_bit, &chunk) == false) {
      return false;
    }
    intermediate += chunk;
    if (((intermediate >> 16) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFFFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint16_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false) {
      return false;
    }
    chunk |= bit << (15 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 16) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFFFF;
  }
  *checksum = intermediate & 0xFFFF;
  return true;
}

bool calculateChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, uint32_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  uint64_t intermediate = 0;
  uint16_t current_bit = start_bit;
  while ((end_bit - current_bit + 1) >= 32) {
    uint32_t chunk;
    if (Packet_Get32(bit_msg, &current_bit, &chunk) == false) {
      return false;
    }
    intermediate += chunk;
    if (((intermediate >> 8) & 1) == 1) {
      intermediate = (intermediate + 1) & 0xFF;
    }
  }
  uint16_t remaining_bits = 1 + end_bit - current_bit;
  uint32_t chunk = 0;
  for (uint16_t i = 0; i < remaining_bits; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, current_bit++, &bit) == false) {
      return false;
    }
    chunk |= bit << (31 - i);
  }
  intermediate += chunk;
  if (((intermediate >> 32) & 1) == 1) {
    intermediate = (intermediate + 1) & 0xFFFFFFFF;
  }
  *checksum = intermediate & 0xFFFFFFFF;
  return true;
}



bool checkCrc8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint8_t theoretical_crc;
  if (calculateCrc8(bit_msg, start_bit, end_bit, &theoretical_crc) == false) {
    return false;
  }
  uint8_t actual_crc;
  end_bit++;
  if (Packet_Get8(bit_msg, &end_bit, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc != theoretical_crc;
  return true;
}

bool checkCrc16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint16_t theoretical_crc;
  if (calculateCrc16(bit_msg, start_bit, end_bit, &theoretical_crc) == false) {
    return false;
  }
  uint16_t actual_crc;
  end_bit++;
  if (Packet_Get16(bit_msg, &end_bit, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc != theoretical_crc;
  return true;
}

bool checkCrc32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint32_t theoretical_crc;
  if (calculateCrc32(bit_msg, start_bit, end_bit, &theoretical_crc) == false) {
    return false;
  }
  uint32_t actual_crc;
  end_bit++;
  if (Packet_Get32(bit_msg, &end_bit, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc != theoretical_crc;
  return true;
}

bool checkChecksum8(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint8_t theoretical_checksum;
  if (calculateChecksum8(bit_msg, start_bit, end_bit, &theoretical_checksum) == false) {
    return false;
  }
  uint8_t actual_checksum;
  end_bit++;
  if (Packet_Get8(bit_msg, &end_bit, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}

bool checkChecksum16(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint16_t theoretical_checksum;
  if (calculateChecksum16(bit_msg, start_bit, end_bit, &theoretical_checksum) == false) {
    return false;
  }
  uint16_t actual_checksum;
  end_bit++;
  if (Packet_Get16(bit_msg, &end_bit, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}

bool checkChecksum32(BitMessage_t* bit_msg, uint16_t start_bit, uint16_t end_bit, bool* error)
{
  *error = true;

  uint32_t theoretical_checksum;
  if (calculateChecksum32(bit_msg, start_bit, end_bit, &theoretical_checksum) == false) {
    return false;
  }
  uint32_t actual_checksum;
  end_bit++;
  if (Packet_Get32(bit_msg, &end_bit, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}
