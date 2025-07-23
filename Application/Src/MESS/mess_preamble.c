/*
 * mess_preamble.c
 *
 *  Created on: Jul 6, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_preamble.h"
#include "mess_packet.h"
#include "mess_main.h"
#include "mess_error_detection.h"
#include "mess_error_correction.h"
#include "cfg_parameters.h"
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  ENCODE_FROM_MEMORY,
  ENCODE_FIXED
} EncodeSource_t;

typedef struct {
  uint8_t start_bit;
  uint8_t bit_len;
  uint16_t value_offset;
  uint16_t flag_offset;

  EncodeSource_t encode_source;

  uint16_t fixed_value;

} PreambleFieldConfig_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/

#define FIELD_ENTRY(start, len, field_name) \
  {start, len, \
   offsetof(PreambleContent_t, field_name) + offsetof(PreambleValue_t, value), \
   offsetof(PreambleContent_t, field_name) + offsetof(PreambleValue_t, valid), \
   ENCODE_FROM_MEMORY, \
   0}

#define FIELD_FIXED_ENTRY(start, len, value) \
  {start, len, 0, 0, ENCODE_FIXED, value}

#define FIELDS_END {0, 0, 0, 0, 0, 0}

/* Private variables ---------------------------------------------------------*/

// All possible preambles

static const PreambleFieldConfig_t custom_fields[] = {
  FIELD_ENTRY(0, 4, modem_id),
  FIELD_ENTRY(4, 4, message_type),
  FIELD_ENTRY(8, 7, cargo_length),
  FIELD_ENTRY(15, 1, is_stationary),
  FIELDS_END
};

/* Private function prototypes -----------------------------------------------*/

bool calculateJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, uint16_t num_bits);
bool decodeJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg);
uint16_t calculateCargoBytes(uint16_t length_index);
bool loadCustomParameters(Message_t* msg);
bool extractBits(BitMessage_t* bit_msg, uint8_t start_bit, uint8_t bit_len, uint16_t* bits);
bool getFields(const PreambleFieldConfig_t** fields, const Message_t* msg);

/* Exported function definitions ---------------------------------------------*/

bool Preamble_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  // Calculate required fields

  uint16_t detection_bits;
  if (ErrorDetection_CheckLength(&detection_bits, cfg->cargo_validation) == false) {
    return false;
  }

  // TODO: check if custom or JANUS
  switch (msg->data_type) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      if (calculateJanusCargoBits(msg, bit_msg, msg->length_bits + detection_bits) == false) {
        return false;
      }
      if (loadCustomParameters(msg) == false) {
        return false;
      }
      break;
    case EVAL: {
      uint16_t eval_bytes;
      if (Param_GetUint16(PARAM_EVAL_MESSAGE_LEN, &eval_bytes) == false) {
        return false;
      }
      if (calculateJanusCargoBits(msg, bit_msg, eval_bytes * 8) == false) {
        return false;
      }
      if (loadCustomParameters(msg) == false) {
        return false;
      }
      break;
    }
    default:
      return false;
  }

  // Add required fields

  const PreambleFieldConfig_t* fields;
  if (getFields(&fields, msg) == false) {
    return false;
  }
  for (uint16_t i = 0; fields[i].bit_len != 0; i++) {
    uint16_t value;
    switch (fields[i].encode_source) {
      case ENCODE_FROM_MEMORY: {
        const uint8_t* base_source = (const uint8_t*)&msg->preamble;
        const uint16_t* value_source = (const uint16_t*)(base_source + fields[i].value_offset);
        const bool* flag_source = (const bool*)(base_source + fields[i].flag_offset);
        if (*flag_source != true) {
          return false;
        }
        value = *value_source;
        break;
      }
      case ENCODE_FIXED:
        value = fields[i].fixed_value;
        break;
      default:
        return false;
    }

    for (uint16_t j = 0; j < fields[i].bit_len; j++) {
      if (Packet_AddBit(bit_msg, (value >> (fields[i].bit_len - j - 1)) & 1) == false) {
        return false;
      }
    }
  }

  // Update length of bit message

  if (Preamble_UpdateNumBits(bit_msg, cfg) == false) {
    return false;
  }

  // Add CRC required by cfg

  if (ErrorDetection_AddDetection(bit_msg, cfg, true) == false) {
    return false;
  }
  return true;
}

bool Preamble_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  memset(&msg->preamble, 0, sizeof(PreambleContent_t));
  // Decode relevant fields
  const PreambleFieldConfig_t* fields;
  if (getFields(&fields, msg) == false) {
    return false;
  }
  for (uint16_t i = 0; fields[i].bit_len != 0; i++) {
    uint16_t value = 0;

    for (uint16_t j = 0; j < fields[i].bit_len; j++) {
      bool bit;
      if (Packet_GetBit(bit_msg, fields[i].start_bit + j, &bit) == false) {
        return false;
      }
      value |= bit << (fields[i].bit_len - j - 1);
    }
    switch (fields[i].encode_source) {
      case ENCODE_FROM_MEMORY: {
        const uint8_t* base_source = (const uint8_t*)&msg->preamble;
        uint16_t* value_source = (uint16_t*)(base_source + fields[i].value_offset);
        bool* flag_source = (bool*)(base_source + fields[i].flag_offset);
        *flag_source = true;
        *value_source = value;
        break;
      }
      case ENCODE_FIXED:
        if (value != fields[i].fixed_value) {
          msg->error_detected = true;
          bit_msg->error_preamble = true;
          bit_msg->error_entire_message = true;
        }
        break;
      default:
        return false;
    }
  }
  if (msg->preamble.message_type.valid == false) {
    return false;
  }
  // Calculate relevant parameters
  switch (msg->preamble.message_type.value) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
    case EVAL:
      if (decodeJanusCargoBits(msg, bit_msg, cfg) == false) {
        return false;
      }
      break;
    default:
      if (decodeJanusCargoBits(msg, bit_msg, cfg) == false) {
        return false;
      }
      msg->preamble.message_type.value = UNKNOWN;
      break;
  }

  // Update length of bit message including ECC
  uint16_t cargo_ecc_len = ErrorCorrection_GetLength(bit_msg->cargo.raw_len, cfg->cargo_ecc_method);
  bit_msg->cargo.ecc_len = cargo_ecc_len;

  // Check CRC
  if (ErrorDetection_CheckDetection(bit_msg, &bit_msg->error_preamble, cfg, true) == false) {
    return false;
  }

  // Final message length calculations
  bit_msg->final_length = bit_msg->preamble.ecc_len + bit_msg->cargo.ecc_len;
  bit_msg->cargo.raw_start_index = bit_msg->preamble.raw_start_index + bit_msg->preamble.raw_len;
  bit_msg->cargo.ecc_start_index = bit_msg->preamble.ecc_start_index + bit_msg->preamble.ecc_len;
  return true;
}

bool Preamble_UpdateNumBits(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  // TODO: update for JANUS
  bit_msg->preamble.raw_len = 16;

  uint16_t detection_bits;
  if (ErrorDetection_CheckLength(&detection_bits, cfg->preamble_validation) == false) {
    return false;
  }
  bit_msg->preamble.raw_len += detection_bits;
  uint16_t preamble_ecc_len = ErrorCorrection_GetLength(bit_msg->preamble.raw_len, cfg->preamble_ecc_method);
  bit_msg->preamble.ecc_len = preamble_ecc_len;
  bit_msg->preamble.raw_start_index = 0;
  bit_msg->preamble.ecc_start_index = 0;
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool calculateJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, uint16_t num_bits)
{
  uint16_t num_bytes = (num_bits + 7) / 8;
  if (num_bytes > 480 || num_bytes == 0) {
    return false;
  }

  uint8_t e = 0;
  uint16_t offset = 32;
  // find the minimum exponent needed to encode the data
  while (offset < num_bytes && e < 3) {
    e++;
    offset += (1 << 5) << e;
  }

  offset -= (1 << 5) << e;

  uint16_t remaining_bytes = num_bytes - offset;
  uint16_t granularity = (1 << e);
  // Round up to the best nearest x
  uint8_t x = (remaining_bytes + granularity - 1) / granularity - 1;
  uint16_t cargo_length = 0;
  // Add bits with format 0eexxxxx
  cargo_length |= ((e & 0x03) << 5);
  cargo_length |= x & 0x1F;
  num_bytes = calculateCargoBytes(cargo_length);
  msg->preamble.cargo_length.value = cargo_length;
  msg->preamble.cargo_length.valid = true;
  bit_msg->cargo.raw_len = num_bytes * 8;
  return true;
}

bool decodeJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (msg->preamble.cargo_length.valid != true) {
    return false;
  }
  uint16_t length_index = msg->preamble.cargo_length.value;
  if (length_index > 127) {
    return false;
  }

  uint16_t num_bytes = calculateCargoBytes(length_index);

  uint16_t cargo_validation_bits;
  if (ErrorDetection_CheckLength(&cargo_validation_bits, cfg->cargo_validation) == false) {
    return false;
  }
  bit_msg->data_len_bits = num_bytes * 8 - cargo_validation_bits;
  bit_msg->cargo.raw_len = num_bytes * 8;
  return true;
}

uint16_t calculateCargoBytes(uint16_t length_index)
{
  uint16_t e = (length_index >> 5) & 0x03;
  uint16_t x = length_index & 0x1F;

  uint16_t num_bytes = (1 << e) * (x + 1);
  // add the offset
  for (uint8_t i = 0; i < e; i++) {
    num_bytes += ((1 << 5) << i);
  }
  return num_bytes;
}

bool loadCustomParameters(Message_t* msg)
{
  uint8_t value;
  if (Param_GetUint8(PARAM_ID, &value) == false) {
    return false;
  }
  if (msg->preamble.modem_id.valid != true) {
    msg->preamble.modem_id.value = value;
    msg->preamble.modem_id.valid = true;
  }
  if (Param_GetUint8(PARAM_STATIONARY_FLAG, &value) == false) {
    return false;
  }
  if (msg->preamble.is_stationary.valid != true) {
    msg->preamble.is_stationary.value = value;
    msg->preamble.is_stationary.valid = true;
  }
  return true;
}

bool extractBits(BitMessage_t* bit_msg, uint8_t start_bit, uint8_t bit_len, uint16_t* bits)
{
  if (Packet_GetChunk(bit_msg, start_bit, bit_len, bits) == false) {
    return false;
  }
  if (*bits >= (1 << bit_len)) {
    return false;
  }
  return true;
}

bool getFields(const PreambleFieldConfig_t** fields, const Message_t* msg)
{
  (void)(msg);
  *fields = custom_fields;
  return true;
}
