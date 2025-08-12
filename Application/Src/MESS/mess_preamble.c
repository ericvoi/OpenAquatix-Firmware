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
#include "mess_cargo.h"
#include "cfg_parameters.h"
#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  ENCODE_FROM_MEMORY,
  ENCODE_FIXED,
  ENCODE_UNUSED
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

#define FIELD_UNUSED(start, len) \
  {start, len, 0, 0, ENCODE_UNUSED, 0}

#define FIELDS_END {0, 0, 0, 0, 0, 0}

/* Private variables ---------------------------------------------------------*/

// All possible preambles

static const PreambleFieldConfig_t custom_fields[] = {
  FIELD_ENTRY(0, 4, modem_id),
  FIELD_ENTRY(4, 4, message_type),
  FIELD_ENTRY(8, 7, cargo_length),
  FIELD_ENTRY(15, 1, is_mobile),
  FIELDS_END
};

// JANUS preambles

static const PreambleFieldConfig_t janus_011_01_sms_fields[] = {
  FIELD_FIXED_ENTRY(0, 4, JANUS_VERSION),
  FIELD_ENTRY(4, 1, is_mobile),
  #define JANUS_011_01_SMS_SCHEDULE_FLAG      (1U)
  FIELD_ENTRY(5, 1, schedule_flag),
  FIELD_ENTRY(6, 1, tx_rx_capable),
  FIELD_ENTRY(7, 1, can_forward),
  #define JANUS_011_01_SMS_CLASS_USER_ID      (11U)
  FIELD_ENTRY(8, 8, class_user_id),       
  #define JANUS_011_01_SMS_APPLICATION_TYPE   (1U)
  FIELD_ENTRY(16, 6, application_type),       
  FIELD_FIXED_ENTRY(22, 1, 0U),       // RPT flag = 0
  FIELD_ENTRY(23, 7, reservation_time),
  FIELD_UNUSED(30, 1),
  FIELD_ENTRY(31, 8, modem_id),
  FIELD_ENTRY(39, 8, destination_id),
  FIELD_ENTRY(47, 2, coding),
  FIELD_ENTRY(49, 2, encryption),
  FIELD_UNUSED(51, 5),
  FIELDS_END
};

/* Private function prototypes -----------------------------------------------*/

static bool calculateJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, uint16_t num_bits);
static bool decodeJanusCargoBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg);
static bool calculateJanusReservationBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg, uint16_t num_bits);
static bool decodeJanusReservationBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg);
static uint16_t calculateCargoBytes(uint16_t length_index);
static bool loadCustomParameters(Message_t* msg);
static bool loadJanusParameters(Message_t* msg);
static bool extractBits(BitMessage_t* bit_msg, uint8_t start_bit, uint8_t bit_len, uint16_t* bits);
static bool getFields(const PreambleFieldConfig_t** fields, const Message_t* msg, const DspConfig_t* cfg);
static bool janusFields(const PreambleFieldConfig_t** fields, JanusMessageData_t janus_message_type);
static bool addPreambleFields(const PreambleFieldConfig_t* fields, const Message_t* msg, BitMessage_t* bit_msg);
static bool extractPreambleFields(const PreambleFieldConfig_t* fields, BitMessage_t* bit_msg, Message_t* msg);
static JanusMessageData_t janusMessageType(uint8_t class_user_id, uint8_t application_type);
static bool cargoLengthCalculation(BitMessage_t* bit_msg, const DspConfig_t* cfg);

static bool addCustomPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool addJanusPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool decodeCustomPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool decodeJanusPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);

/* Exported function definitions ---------------------------------------------*/

bool Preamble_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      return addCustomPreamble(bit_msg, msg, cfg);
    case PROTOCOL_JANUS:
      return addJanusPreamble(bit_msg, msg, cfg);
    default:
      return false;
  }
}

bool Preamble_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      return decodeCustomPreamble(bit_msg, msg, cfg);
    case PROTOCOL_JANUS:
      return decodeJanusPreamble(bit_msg, msg, cfg);
    default:
      return false;
  }
}

bool Preamble_UpdateNumBits(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  bit_msg->preamble.raw_len = (cfg->protocol == PROTOCOL_CUSTOM) ? 
                              PACKET_PREAMBLE_LENGTH_BITS : JANUS_PREAMBLE_LEN;

  uint16_t detection_bits;
  if (ErrorDetection_CheckLength(&detection_bits, cfg->preamble_validation) == false) {
    return false;
  }
  bit_msg->preamble.raw_len += detection_bits;
  uint16_t preamble_ecc_len = ErrorCorrection_CodedLength(bit_msg->preamble.raw_len, cfg->preamble_ecc_method);
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
  bit_msg->cargo.ecc_len = ErrorCorrection_CodedLength(bit_msg->cargo.raw_len, cfg->cargo_ecc_method);
  return true;
}

bool calculateJanusReservationBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg, uint16_t num_bits)
{
  num_bits = ErrorCorrection_CodedLength(num_bits, cfg->cargo_ecc_method);
  float required_time = num_bits / cfg->baud_rate;
  // reservation_time_index = (m + 16)*2^e
  uint16_t reservation_time_index = (uint16_t) ceilf(required_time * 25.0f / 4.0f + 14.0f);

  uint8_t e = 0;
  while ((15 + 16) * (1 << e) < reservation_time_index) {
    e++;
    if (e > 7) return false;
  }
  uint8_t m = (uint8_t) ceil(((float) reservation_time_index) / ((float) (1 << e)) - 16.0f);
  if (m > 15) {
    return false;
  }
  uint16_t reservation_length = ((e & 0x07) << 4) | (m & 0x0F);
  msg->preamble.reservation_time.value = reservation_length;
  msg->preamble.reservation_time.valid = true;
  float reservation_time = ((m + 16.0f) * (1 << e) - 14.0f) * (4.0f / 25.0f);
  bit_msg->cargo.ecc_len = (uint16_t) roundf(reservation_time * cfg->baud_rate);
  bit_msg->cargo.raw_len = ErrorCorrection_UncodedLength(bit_msg->cargo.ecc_len, cfg->cargo_ecc_method);
  return true;
}

bool decodeJanusReservationBits(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (msg->preamble.reservation_time.valid == false) {
    return false;
  }
  uint16_t reservation_time_index = msg->preamble.reservation_time.value;
  if (reservation_time_index > 127) {
    return false;
  }

  uint8_t m = reservation_time_index & 0x0F;
  uint8_t e = (reservation_time_index >> 4) & 0x07;
  float reservation_time = ((m + 16.0f) * (1 << e) - 14.0f) * (4.0f / 25.0f);
  uint16_t num_bits = (uint16_t) roundf(reservation_time * cfg->baud_rate);

  uint16_t cargo_validation_bits;
  if (ErrorDetection_CheckLength(&cargo_validation_bits, cfg->cargo_validation) == false) {
    return false;
  }
  bit_msg->cargo.ecc_len = num_bits;
  bit_msg->cargo.raw_len = ErrorCorrection_UncodedLength(num_bits, cfg->cargo_ecc_method);
  bit_msg->data_len_bits = bit_msg->cargo.raw_len - cargo_validation_bits;
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
  // Checks for validity since configuration values can be overridden by
  // feedback tests in which case the value should not be overwritten
  if (msg->preamble.modem_id.valid != true) {
    msg->preamble.modem_id.value = value;
    msg->preamble.modem_id.valid = true;
  }
  if (Param_GetUint8(PARAM_STATIONARY_FLAG, &value) == false) {
    return false;
  }
  if (msg->preamble.is_mobile.valid != true) {
    msg->preamble.is_mobile.value = value;
    msg->preamble.is_mobile.valid = true;
  }
  return true;
}

bool loadJanusParameters(Message_t* msg)
{
  uint8_t value;
  if (Param_GetUint8(PARAM_TX_RX_ABILITY, &value) == false) {
    return false;
  }
  // Checks for validity since configuration values can be overridden by
  // feedback tests in which case the value should not be overwritten
  if (msg->preamble.tx_rx_capable.valid != true) {
    msg->preamble.tx_rx_capable.value = value;
    msg->preamble.tx_rx_capable.valid = true;
  }
  if (Param_GetUint8(PARAM_FORWARD_CAPABILITY, &value) == false) {
    return false;
  }
  if (msg->preamble.can_forward.valid != true) {
    msg->preamble.can_forward.value = value;
    msg->preamble.can_forward.valid = true;
  }
  if (Param_GetUint8(PARAM_JANUS_ID, &value) == false) {
    return false;
  }
  if (msg->preamble.modem_id.valid != true) {
    msg->preamble.modem_id.value = value;
    msg->preamble.modem_id.valid = true;
  }
  if (Param_GetUint8(PARAM_JANUS_DESTINATION, &value) == false) {
    return false;
  }
  if (msg->preamble.destination_id.valid != true) {
    msg->preamble.destination_id.value = value;
    msg->preamble.destination_id.valid = true;
  }
  if (Param_GetUint8(PARAM_CODING, &value) == false) {
    return false;
  }
  if (msg->preamble.coding.valid != true) {
    msg->preamble.coding.value = value;
    msg->preamble.coding.valid = true;
  }
  if (Param_GetUint8(PARAM_ENCRYPTION, &value) == false) {
    return false;
  }
  if (msg->preamble.encryption.valid != true) {
    msg->preamble.encryption.value = value;
    msg->preamble.encryption.valid = true;
  }
  if (Param_GetUint8(PARAM_STATIONARY_FLAG, &value) == false) {
    return false;
  }
  if (msg->preamble.is_mobile.valid != true) {
    msg->preamble.is_mobile.value = value;
    msg->preamble.is_mobile.valid = true;
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

bool getFields(const PreambleFieldConfig_t** fields, const Message_t* msg, const DspConfig_t* cfg)
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      *fields = custom_fields;
      return true;
    case PROTOCOL_JANUS:
      return janusFields(fields, msg->janus_data_type);
    default:
    return false;
  }
}

bool janusFields(const PreambleFieldConfig_t** fields, JanusMessageData_t janus_message_type)
{
  switch (janus_message_type) {
    case JANUS_011_01_SMS:
      *fields = janus_011_01_sms_fields;
      return true;
    default:
      return false;
  }
}

bool addPreambleFields(const PreambleFieldConfig_t* fields, const Message_t* msg, BitMessage_t* bit_msg)
{
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
      case ENCODE_UNUSED:
        value = 0U;
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
  return true;
}

bool extractPreambleFields(const PreambleFieldConfig_t* fields, BitMessage_t* bit_msg, Message_t* msg)
{
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
      case ENCODE_UNUSED:
        break;
      default:
        return false;
    }
  }
  return true;
}

JanusMessageData_t janusMessageType(uint8_t class_user_id, uint8_t application_type)
{
  if (class_user_id == JANUS_011_01_SMS_CLASS_USER_ID && application_type == JANUS_011_01_SMS_APPLICATION_TYPE) {
    return JANUS_011_01_SMS;
  }
  return JANUS_UNKNOWN;
}

bool cargoLengthCalculation(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  uint16_t cargo_ecc_len = ErrorCorrection_CodedLength(bit_msg->cargo.raw_len, cfg->cargo_ecc_method);
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

bool addCustomPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  // Calculate required fields

  uint16_t detection_bits;
  if (ErrorDetection_CheckLength(&detection_bits, cfg->cargo_validation) == false) {
    return false;
  }
  
  if (loadCustomParameters(msg) == false) {
    return false;
  }

  switch (msg->data_type) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      if (calculateJanusCargoBits(msg, bit_msg, msg->length_bits + detection_bits) == false) {
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
      break;
    }
    default:
      return false;
  }

  // Add required fields

  const PreambleFieldConfig_t* fields;
  if (getFields(&fields, msg, cfg) == false) {
    return false;
  }
  if (addPreambleFields(fields, msg, bit_msg) == false) {
    return false;
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

bool addJanusPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  uint16_t detection_bits;
  if (ErrorDetection_CheckLength(&detection_bits, cfg->cargo_validation) == false) {
    return false;
  }

  if (loadJanusParameters(msg) == false) {
    return false;
  }

  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      uint16_t raw_coded_len = Cargo_RawCodedLength(msg->length_bits, msg->preamble.coding.value) + detection_bits;
      if (calculateJanusReservationBits(msg, bit_msg, cfg, raw_coded_len) == false) {
        return false;
      }
      msg->preamble.class_user_id.value = JANUS_011_01_SMS_CLASS_USER_ID;
      msg->preamble.application_type.value = JANUS_011_01_SMS_APPLICATION_TYPE;
      msg->preamble.schedule_flag.value = JANUS_011_01_SMS_SCHEDULE_FLAG;
      msg->preamble.class_user_id.valid = true;
      msg->preamble.application_type.valid = true;
      msg->preamble.schedule_flag.valid = true;

      // Encryption is a 3 bit value but 011 01 only allots 2 bits for encryption
      if (msg->preamble.encryption.value > 3) {
        return false;
      }
      break;
    default:
      return false;
  }

  const PreambleFieldConfig_t* fields;
  if (getFields(&fields, msg, cfg) == false) {
    return false;
  }
  if (addPreambleFields(fields, msg, bit_msg) == false) {
    return false;
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

bool decodeCustomPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  memset(&msg->preamble, 0, sizeof(PreambleContent_t));
  // Decode relevant fields
  const PreambleFieldConfig_t* fields;
  if (getFields(&fields, msg, cfg) == false) {
    return false;
  }
  if (extractPreambleFields(fields, bit_msg, msg) == false) {
    return false;
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

  if (cargoLengthCalculation(bit_msg, cfg) == false) {
    return false;
  }
  return true;
}

bool decodeJanusPreamble(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  memset(&msg->preamble, 0, sizeof(PreambleContent_t));

  uint8_t class_user_id;
  uint8_t application_type;
  uint16_t start_pos = 8;
  if (Packet_Get8BitChunk(bit_msg, &start_pos, 8, &class_user_id) == false) {
    return false;
  }
  start_pos = 16;
  if (Packet_Get8BitChunk(bit_msg, &start_pos, 6, &application_type) == false) {
    return false;
  }

  JanusMessageData_t message_type = janusMessageType(class_user_id, application_type);
  if (message_type == JANUS_UNKNOWN) return false;

  msg->janus_data_type = message_type;

  const PreambleFieldConfig_t* fields;
  if (janusFields(&fields, message_type) == false) {
    return false;
  }

  if (extractPreambleFields(fields, bit_msg, msg) == false) {
    return false;
  }

  switch (message_type) {
    case JANUS_011_01_SMS:
      if (decodeJanusReservationBits(msg, bit_msg, cfg) == false) {
        return false;
      }
      break;
    case JANUS_UNKNOWN:
      bit_msg->data_len_bits = 0;
      bit_msg->cargo.raw_len = 0;
      break;
    default:
      return false;
  }

  if (cargoLengthCalculation(bit_msg, cfg) == false) {
    return false;
  }
  return true;
}
