/*
 * mess_cargo.c
 *
 *  Created on: Jul 9, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_cargo.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "mess_main.h"
#include "mess_error_detection.h"
#include "mess_error_correction.h"
#include "mess_evaluate.h"
#include <stdbool.h>
#include <ctype.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static const uint8_t ais_6_ascii8_lut[] = {
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',   // 0x00 - 0x0F
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_',  // 0x10 - 0x1F
  ' ', '!', '\"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', // 0x20 - 0x2F
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?'    // 0x30 - 0x3F
};

/* Private function prototypes -----------------------------------------------*/

static bool addCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool addJanusCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool addDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool addJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static bool addCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg);
static bool extractCustomCargo(BitMessage_t* bit_msg, Message_t* msg);
static bool extractDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg);
static bool extractDataJanusCargo(BitMessage_t* bit_msg, Message_t* msg);
static bool extractJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg);
static bool extractCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg);

static bool encodeAsAscii6(uint8_t ascii_8, uint8_t* ascii_6);
static bool decodeAscii6(uint8_t ascii_6, uint8_t* ascii_8);

/* Exported function definitions ---------------------------------------------*/

bool Cargo_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      if (addCustomCargo(bit_msg, msg, cfg) == false) {
        return false;
      }
      break;
    case PROTOCOL_JANUS:
      if (addJanusCargo(bit_msg, msg, cfg) == false) {
        return false;
      }
      break;
    default:
      return false;
  }
  
  bit_msg->cargo.ecc_len = ErrorCorrection_CodedLength(bit_msg->cargo.raw_len, 
                                                       cfg->cargo_ecc_method);
  bit_msg->combined_message_len = bit_msg->cargo.raw_len + bit_msg->preamble.raw_len;
  bit_msg->cargo.raw_start_index = bit_msg->preamble.raw_start_index
                                 + bit_msg->preamble.raw_len;
  bit_msg->cargo.ecc_start_index = bit_msg->preamble.ecc_start_index
                                 + bit_msg->preamble.ecc_len;
  bit_msg->final_length = bit_msg->preamble.ecc_len + bit_msg->cargo.ecc_len;
  bit_msg->bit_count = bit_msg->preamble.raw_len + bit_msg->cargo.raw_len;
  return true;
}

bool Cargo_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      msg->uncoded_data_len = bit_msg->data_len_bits;
      return extractCustomCargo(bit_msg, msg);
    case PROTOCOL_JANUS:
      return extractDataJanusCargo(bit_msg, msg);
    default:
      return false;
  }
}

// Assumes that the input data length is a multiple of 8
uint16_t Cargo_RawCodedLength(uint16_t uncoded_len, CodingInfo_t coding_method)
{
  switch (coding_method) {
    case CODING_ASCII8:
    case CODING_UTF8:
      return uncoded_len;
    case CODING_ASCII7:
      return uncoded_len * 7 / 8;
    case CODING_ASCII6:
      return uncoded_len * 6 / 8;
    default:
      return 0;
  }
}

uint16_t Cargo_RawUncodedLength(uint16_t coded_len, CodingInfo_t coding_method)
{
  switch (coding_method) {
    case CODING_ASCII8:
    case CODING_UTF8:
      return coded_len;
    case CODING_ASCII7:
      return coded_len * 8 / 7;
    case CODING_ASCII6:
      return coded_len * 8 / 6;
    default:
      return 0;
  }
}

/* Private function definitions ----------------------------------------------*/

bool addCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg) 
{
  switch (msg->data_type) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      return addDataCustomCargo(bit_msg, msg, cfg);
    case EVAL:
      return Evaluate_AddCargo(bit_msg);
    default:
      return false;
  }
}

bool addJanusCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      return addJanus_11_01_Cargo(bit_msg, msg, cfg);
    default:
      return false;
  }
}

bool addDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < msg->length_bits; i++) {
    uint16_t byte_index = i / 8;
    uint16_t bit_index = i % 8;
    bool bit = (msg->data[byte_index] & (1 << (7 - bit_index))) != 0;
    if (Packet_AddBit(bit_msg, bit) == false)
      return false;
  }
  uint16_t error_detection_bits;
  if (ErrorDetection_CheckLength(&error_detection_bits, 
                                 cfg->cargo_validation) == false) {
    return false;
  }
  bit_msg->data_len_bits = bit_msg->cargo.raw_len - error_detection_bits;
  // Skip any padding bits
  bit_msg->bit_count = bit_msg->data_len_bits + bit_msg->cargo.raw_start_index;
  if (ErrorDetection_AddDetection(bit_msg, cfg, false) == false) {
    return false;
  }
  return true;
}

// JANUS class user id 11 and application type 1 allows the sending and
// receiving of text messages with variable coding and encryption
bool addJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  if (addCodedEncryptedData(msg->preamble.coding.value, msg->preamble.encryption.value, bit_msg, msg) == false) {
    return false;
  }

  uint16_t error_detection_bits;
  if (ErrorDetection_CheckLength(&error_detection_bits, 
                                 cfg->cargo_validation) == false) {
    return false;
  }
  bit_msg->data_len_bits = bit_msg->cargo.raw_len - error_detection_bits;
  // Skip any padding bits
  bit_msg->bit_count = bit_msg->data_len_bits + bit_msg->cargo.raw_start_index;
  if (ErrorDetection_AddDetection(bit_msg, cfg, false) == false) {
    return false;
  }
  return true;
}

// Encryption not yet implemented
bool addCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg)
{
  if (encryption != ENCRYPTION_NONE) {
    return false;
  }

  for (uint16_t i = 0; i < msg->length_bits / 8; i++) {
    uint8_t coded_value;
    uint8_t coded_len;
    switch (coding) {
      case CODING_ASCII8:
      case CODING_UTF8:
        coded_value = msg->data[i];
        coded_len = 8;
        break;
      case CODING_ASCII7:
        if (msg->data[i] > 127) return false;
        coded_value = msg->data[i];
        coded_len = 7;
        break;
      case CODING_ASCII6:
        if (encodeAsAscii6(toupper(msg->data[i]), &coded_value) == false) {
          return false;
        }
        coded_len = 6;
        break;
      default:
        return false;
    }
    for (uint16_t j = 0; j < coded_len; j++) {
      bool bit = (coded_value & (1 << (coded_len - 1 - j))) != 0;
      if (Packet_AddBit(bit_msg, bit) == false)
        return false;
    }
  }
  return true;
}

bool extractCustomCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  switch (msg->preamble.message_type.value) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      return extractDataCustomCargo(bit_msg, msg);
    case EVAL:
      return Evaluate_CodedBer(&msg->eval_info, bit_msg);
    default:
      return false;
  }
}

bool extractDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  // data_len_bytes is restricted to be a multiple of 8
  uint16_t len_bytes = bit_msg->data_len_bits / 8;

  uint16_t start_position = bit_msg->cargo.raw_start_index;

  for (uint16_t i = 0; i < len_bytes; i++) {
    if (Packet_Get8(bit_msg, &start_position, msg->data + i) == false) {
      return false;
    }
  }
  return true;
}

bool extractDataJanusCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      msg->uncoded_data_len = Cargo_RawUncodedLength(bit_msg->data_len_bits, msg->preamble.coding.value);
      return extractJanus_11_01_Cargo(bit_msg, msg);
    default:
      return false;
  }
}

bool extractJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg)
{
  if (extractCodedEncryptedData(msg->preamble.coding.value, msg->preamble.encryption.value, bit_msg, msg) == false) {
    return false;
  }
  return true;
}

bool extractCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg)
{
  if (encryption != ENCRYPTION_NONE) return false;

  uint8_t chunk_size;
  switch (coding) {
    case CODING_ASCII8:
    case CODING_UTF8:
      chunk_size = 8;
      break;
    case CODING_ASCII7:
      chunk_size = 7;
      break;
    case CODING_ASCII6:
      chunk_size = 6;
      break;
    default:
      return false;
  }

  // Does not include error detection bits
  uint16_t remaining_length = bit_msg->data_len_bits;

  uint16_t msg_byte_index = 0;
  uint16_t bit_msg_bit_index = bit_msg->cargo.raw_start_index;

  while (remaining_length >= chunk_size) {
    uint8_t coded_byte = 0;
    uint8_t uncoded_byte = 0;
    for (uint8_t i = 0; i < chunk_size; i++) {
      bool bit;
      if (Packet_GetBit(bit_msg, bit_msg_bit_index, &bit) == false) {
        return false;
      }
      coded_byte |= bit << (chunk_size - 1 - i);
      bit_msg_bit_index++;
    }
    switch (coding) {
      case CODING_ASCII8:
      case CODING_UTF8:
      case CODING_ASCII7:
        uncoded_byte = coded_byte;
        break;
      case CODING_ASCII6:
        if (decodeAscii6(coded_byte, &uncoded_byte) == false) {
          return false;
        }
        break;
      default:
        return false;
    }
    msg->data[msg_byte_index++] = uncoded_byte;
    remaining_length -= chunk_size;
  }
  return true;
}

bool encodeAsAscii6(uint8_t ascii_8, uint8_t* ascii_6)
{
  for (uint8_t i = 0; i < sizeof(ais_6_ascii8_lut) / sizeof(ais_6_ascii8_lut[0]); i++) {
    if (ascii_8 == ais_6_ascii8_lut[i]) {
      *ascii_6 = i;
      return true;
    }
  }
  return false;
}

bool decodeAscii6(uint8_t ascii_6, uint8_t* ascii_8)
{
  if (ascii_6 > (sizeof(ais_6_ascii8_lut) / sizeof(ais_6_ascii8_lut[0]))) {
    return false;
  }
  *ascii_8 = ais_6_ascii8_lut[ascii_6];
  return true;
}
