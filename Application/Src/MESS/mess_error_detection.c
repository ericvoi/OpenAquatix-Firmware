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

// x^2 + x^1 + 1
#define CRC_8_POLYNOMIAL  0x07U 
// x^12 + x^5 + 1
#define CRC_16_POLYNOMIAL 0x1021U
// x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x^1 + 1
#define CRC_32_POLYNOMIAL 0x04C11DB7U 

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

bool calculateCrc8(BitMessage_t* bit_msg, uint8_t* crc);
bool calculateCrc16(BitMessage_t* bit_msg, uint16_t* crc);
bool calculateCrc32(BitMessage_t* bit_msg, uint32_t* crc);
bool calculateChecksum8(BitMessage_t* bit_msg, uint8_t* checksum);
bool calculateChecksum16(BitMessage_t* bit_msg, uint16_t* checksum);
bool calculateChecksum32(BitMessage_t* bit_msg, uint32_t* checksum);

bool checkCrc8(BitMessage_t* bit_msg, bool* error);
bool checkCrc16(BitMessage_t* bit_msg, bool* error);
bool checkCrc32(BitMessage_t* bit_msg, bool* error);
bool checkChecksum8(BitMessage_t* bit_msg, bool* error);
bool checkChecksum16(BitMessage_t* bit_msg, bool* error);
bool checkChecksum32(BitMessage_t* bit_msg, bool* error);

/* Exported function definitions ---------------------------------------------*/

bool ErrorDetection_AddDetection(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  switch (cfg->error_detection_method) {
    case CRC_8:
      uint8_t crc_8;
      bit_msg->final_length += 8;
      bit_msg->combined_message_len += 8;
      if (calculateCrc8(bit_msg, &crc_8) == false) {
        return false;
      }
      return Packet_Add8(bit_msg, crc_8);
    case CRC_16:
      uint16_t crc_16;
      bit_msg->final_length += 16;
      bit_msg->combined_message_len += 16;
      if (calculateCrc16(bit_msg, &crc_16) == false) {
        return false;
      }
      return Packet_Add16(bit_msg, crc_16);
    case CRC_32:
      uint32_t crc_32;
      bit_msg->final_length += 32;
      bit_msg->combined_message_len += 32;
      if (calculateCrc32(bit_msg, &crc_32) == false) {
        return false;
      }
      return Packet_Add32(bit_msg, crc_32);
    case CHECKSUM_8:
      uint8_t checksum_8;
      bit_msg->final_length += 8;
      bit_msg->combined_message_len += 8;
      if (calculateChecksum8(bit_msg, &checksum_8) == false) {
        return false;
      }
      return Packet_Add8(bit_msg, checksum_8);
    case CHECKSUM_16:
      uint16_t checksum_16;
      bit_msg->final_length += 16;
      bit_msg->combined_message_len += 16;
      if (calculateChecksum16(bit_msg, &checksum_16) == false) {
        return false;
      }
      return Packet_Add16(bit_msg, checksum_16);
    case CHECKSUM_32:
      uint32_t checksum_32;
      bit_msg->final_length += 32;
      bit_msg->combined_message_len += 32;
      if (calculateChecksum32(bit_msg, &checksum_32) == false) {
        return false;
      }
      return Packet_Add32(bit_msg, checksum_32);
    case NO_ERROR_DETECTION:
      return true;
    default:
      return false;
  }
}

bool ErrorDetection_CheckDetection(BitMessage_t* bit_msg, bool* error, const DspConfig_t* cfg)
{
  switch (cfg->error_detection_method) {
    case CRC_8:
      return checkCrc8(bit_msg, error);
    case CRC_16:
      return checkCrc16(bit_msg, error);
    case CRC_32:
      return checkCrc32(bit_msg, error);
    case CHECKSUM_8:
      return checkChecksum8(bit_msg, error);
    case CHECKSUM_16:
      return checkChecksum16(bit_msg, error);
    case CHECKSUM_32:
      return checkChecksum32(bit_msg, error);
    case NO_ERROR_DETECTION:
      return false;
    default:
      return false;
  }
}

bool ErrorDetection_CheckLength(uint16_t* length, const DspConfig_t* cfg)
{
  switch (cfg->error_detection_method) {
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

bool calculateCrc8(BitMessage_t* bit_msg, uint8_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

  uint8_t polynomial = CRC_8_POLYNOMIAL;
  *crc = 0;

  uint16_t byte_count = (bit_msg->combined_message_len - 8) / 8;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 8) % 8;

  for (uint16_t i = 0; i < byte_count; i++) {
    *crc ^= bit_msg->data[i];

    for (uint16_t j = 0; j < 8; j++) {
      if (*crc & 0x80) {
        *crc = (*crc << 1) ^ polynomial;
      }
      else {
        *crc = *crc << 1;
      }
    }
  }

  if (remaining_bits > 0) {
    uint8_t last_byte = bit_msg->data[byte_count] & (0xFF << (8 - remaining_bits));
    *crc ^= last_byte;

    for (uint8_t i = 0; i < remaining_bits; i++) {
      if (*crc & 0x80) {
        *crc = (*crc << 1) ^ polynomial;
      }
      else {
        *crc = *crc << 1;
      }
    }
  }
  return true;
}

bool calculateCrc16(BitMessage_t* bit_msg, uint16_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

  uint16_t polynomial = CRC_16_POLYNOMIAL;
  *crc = 0xFFFF;

  uint16_t byte_count = (bit_msg->combined_message_len - 16) / 8;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 16) % 8;

  for (uint16_t i = 0; i < byte_count; i++) {
    *crc ^= (uint16_t) bit_msg->data[i] << 8;

    for (uint16_t j = 0; j < 8; j++) {
      if (*crc & 0x8000) {
        *crc = (*crc << 1) ^ polynomial;
      }
      else {
        *crc = *crc << 1;
      }
    }
  }

  if (remaining_bits > 0) {
    uint8_t last_byte = bit_msg->data[byte_count] & (0xFF << (8 - remaining_bits));
    *crc ^= (uint16_t)last_byte << 8;

    for (uint16_t j = 0; j < remaining_bits; j++) {
      if (*crc & 0x8000) {
        *crc = (*crc << 1) ^ polynomial;
      }
      else {
        *crc = *crc << 1;
      }
    }
  }
  return true;
}

bool calculateCrc32(BitMessage_t* bit_msg, uint32_t* crc)
{
  if (bit_msg == NULL || crc == NULL) {
    return false;
  }

  uint32_t polynomial = CRC_32_POLYNOMIAL;
  *crc = 0xFFFFFFFF;

  uint16_t byte_count = (bit_msg->combined_message_len - 32) / 8;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 32) % 8;

  for (uint16_t i = 0; i < byte_count; i++) {
    *crc ^= (uint32_t) bit_msg->data[i] << 24;

    for (uint16_t j = 0; j < 8; j++) {
      if (*crc & 0x80000000) {
        *crc = (*crc << 1) ^ polynomial;
      }
      else {
        *crc = *crc << 1;
      }
    }
  }

  if (remaining_bits > 0) {
    uint8_t last_byte = bit_msg->data[byte_count] & (0xFF << (8 - remaining_bits));
    *crc ^= ((uint32_t)last_byte << 24);

    for (size_t j = 0; j < remaining_bits; j++) {
      if (*crc & 0x80000000) {
        *crc = (*crc << 1) ^ 0x04C11DB7;
      } else {
        *crc = *crc << 1;
      }
    }
  }
  *crc = ~*crc;
  return true;
}

bool calculateChecksum8(BitMessage_t* bit_msg, uint8_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  *checksum = 0;

  uint16_t chunk_count = (bit_msg->combined_message_len - 8) / 8;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 8) % 8;

  for (uint16_t i = 0; i < chunk_count; i++) {
    uint8_t chunk = *(((uint8_t*) bit_msg->data) + i);
    *checksum += chunk;
  }

  if (remaining_bits > 0) {
    uint8_t chunk = bit_msg->data[chunk_count];
    *checksum += chunk;
  }
  return true;
}

bool calculateChecksum16(BitMessage_t* bit_msg, uint16_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  *checksum = 0;

  uint16_t chunk_count = (bit_msg->combined_message_len - 16) / 16;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 16) % 16;

  for (uint16_t i = 0; i < chunk_count; i++) {
    uint16_t chunk = *(((uint16_t*) bit_msg->data) + i);
    *checksum += chunk;
  }

  if (remaining_bits > 0) {
    uint16_t chunk = bit_msg->data[chunk_count * 2] << 8;
    if (remaining_bits > 8) {
      chunk |= bit_msg->data[chunk_count * 2 + 1];
    }
    *checksum += chunk;
  }
  return true;
}

bool calculateChecksum32(BitMessage_t* bit_msg, uint32_t* checksum)
{
  if (bit_msg == NULL || checksum == NULL) {
    return false;
  }

  *checksum = 0;

  uint16_t chunk_count = (bit_msg->combined_message_len - 32) / 32;
  uint16_t remaining_bits = (bit_msg->combined_message_len - 32) % 32;

  for (uint16_t i = 0; i < chunk_count; i++) {
    uint32_t chunk = *(((uint32_t*) bit_msg->data) + i);
    *checksum += chunk;
  }

  if (remaining_bits > 0) {
    uint32_t chunk = bit_msg->data[chunk_count * 4] << 24;
    if (remaining_bits > 8) {
      chunk |= bit_msg->data[chunk_count * 4 + 1] << 16;
      if (remaining_bits > 16) {
        chunk |= bit_msg->data[chunk_count * 4 + 2] << 8;
        if (remaining_bits > 24) {
          chunk |= bit_msg->data[chunk_count * 4 + 3];
        }
      }
    }
    *checksum += chunk;
  }
  return true;
}



bool checkCrc8(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint8_t theoretical_crc;
  if (calculateCrc8(bit_msg, &theoretical_crc) == false) {
    return false;
  }
  uint8_t actual_crc;
  uint16_t start_position = bit_msg->combined_message_len - 8;
  if (Packet_Get8(bit_msg, &start_position, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc == theoretical_crc;
  return true;
}

bool checkCrc16(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint16_t theoretical_crc;
  if (calculateCrc16(bit_msg, &theoretical_crc) == false) {
    return false;
  }
  uint16_t actual_crc;
  uint16_t start_position = bit_msg->combined_message_len - 16;
  if (Packet_Get16(bit_msg, &start_position, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc != theoretical_crc;
  return true;
}

bool checkCrc32(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint32_t theoretical_crc;
  if (calculateCrc32(bit_msg, &theoretical_crc) == false) {
    return false;
  }
  uint32_t actual_crc;
  uint16_t start_position = bit_msg->combined_message_len - 32;
  if (Packet_Get32(bit_msg, &start_position, &actual_crc) == false) {
    return false;
  }

  *error = actual_crc != theoretical_crc;
  return true;
}

bool checkChecksum8(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint8_t theoretical_checksum;
  if (calculateChecksum8(bit_msg, &theoretical_checksum) == false) {
    return false;
  }
  uint8_t actual_checksum;
  uint16_t start_position = bit_msg->combined_message_len - 8;
  if (Packet_Get8(bit_msg, &start_position, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}

bool checkChecksum16(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint16_t theoretical_checksum;
  if (calculateChecksum16(bit_msg, &theoretical_checksum) == false) {
    return false;
  }
  uint16_t actual_checksum;
  uint16_t start_position = bit_msg->combined_message_len - 16;
  if (Packet_Get16(bit_msg, &start_position, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}

bool checkChecksum32(BitMessage_t* bit_msg, bool* error)
{
  *error = true;

  uint32_t theoretical_checksum;
  if (calculateChecksum32(bit_msg, &theoretical_checksum) == false) {
    return false;
  }
  uint32_t actual_checksum;
  uint16_t start_position = bit_msg->combined_message_len - 32;
  if (Packet_Get32(bit_msg, &start_position, &actual_checksum) == false) {
    return false;
  }

  *error = actual_checksum != theoretical_checksum;
  return true;
}
