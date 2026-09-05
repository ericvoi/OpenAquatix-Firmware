/*
 * mess_cargo.c
 *
 *  Created on: Jul 9, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_cargo.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "mess_main.h"
#include "mess_error_detection.h"
#include "mess_error_correction.h"
#include "mess_evaluate.h"
#include "error_manager.h"
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

static void addCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg, bool* no_cargo);
static void addJanusCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static void addDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static void addJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg);
static void addCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg);
static void extractCustomCargo(BitMessage_t* bit_msg, Message_t* msg);
static void extractDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg);
static void extractDataJanusCargo(BitMessage_t* bit_msg, Message_t* msg);
static void extractJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg);
static void extractCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg);

static void encodeAsAscii6(uint8_t ascii_8, uint8_t* ascii_6);
static void decodeAscii6(uint8_t ascii_6, uint8_t* ascii_8);

/* Exported function definitions ---------------------------------------------*/

void Cargo_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  RETURN_IF_ERROR_PRESENT();
  bool no_cargo = false;
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      RETURN_IF_ERROR_PRESENT(addCustomCargo(bit_msg, msg, cfg, &no_cargo));
      break;
    case PROTOCOL_JANUS:
      RETURN_IF_ERROR_PRESENT(addJanusCargo(bit_msg, msg, cfg));
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
      break;
  }

  ErrorCorrectionMethod_t ecc_method = (no_cargo) ? (NO_ECC) : (cfg->cargo_ecc_method);
  
  bit_msg->cargo.ecc_len = ErrorCorrection_CodedLength(bit_msg->cargo.raw_len, 
                                                       ecc_method);
  bit_msg->combined_message_len = bit_msg->cargo.raw_len + bit_msg->preamble.raw_len;
  bit_msg->cargo.raw_start_index = bit_msg->preamble.raw_start_index
                                 + bit_msg->preamble.raw_len;
  bit_msg->cargo.ecc_start_index = bit_msg->preamble.ecc_start_index
                                 + bit_msg->preamble.ecc_len;
  bit_msg->final_length = bit_msg->preamble.ecc_len + bit_msg->cargo.ecc_len;
  bit_msg->bit_count = bit_msg->preamble.raw_len + bit_msg->cargo.raw_len;
}

void Cargo_Decode(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  RETURN_IF_ERROR_PRESENT();
  switch (cfg->protocol) {
    case PROTOCOL_CUSTOM:
      msg->uncoded_data_len = bit_msg->data_len_bits;
      RETURN_IF_ERROR_PRESENT(extractCustomCargo(bit_msg, msg));
      break;
    case PROTOCOL_JANUS:
      RETURN_IF_ERROR_PRESENT(extractDataJanusCargo(bit_msg, msg));
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
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
      REGISTER_ERROR_NON_VOID(ERROR_UNHANDLED_CASE, 0);
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
      REGISTER_ERROR_NON_VOID(ERROR_UNHANDLED_CASE, 0);
      return 0;
  }
}

/* Private function definitions ----------------------------------------------*/

void addCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg, bool* no_cargo) 
{
  switch (msg->data_type) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      RETURN_IF_ERROR_PRESENT(addDataCustomCargo(bit_msg, msg, cfg));
      break;
    case EVAL:
      RETURN_IF_ERROR_PRESENT(Evaluate_AddCargo(bit_msg));
      break;
    case RANGING_REQUEST:
    case RANGING_RESPONSE:
      bit_msg->data_len_bits = 0;
      bit_msg->bit_count = bit_msg->cargo.raw_start_index;
      *no_cargo = true;
      break;
    case CHANNEL_TVIR:
      
      *no_cargo == true;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void addJanusCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      RETURN_IF_ERROR_PRESENT(addJanus_11_01_Cargo(bit_msg, msg, cfg));
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void addDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < msg->length_bits; i++) {
    uint16_t byte_index = i / 8;
    uint16_t bit_index = i % 8;
    bool bit = (msg->data[byte_index] & (1 << (7 - bit_index))) != 0;
    if (Packet_AddBit(bit_msg, bit) == false)
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
  }
  uint16_t error_detection_bits;
  ErrorDetection_CheckLength(&error_detection_bits, cfg->cargo_validation);
  bit_msg->data_len_bits = bit_msg->cargo.raw_len - error_detection_bits;
  // Skip any padding bits
  bit_msg->bit_count = bit_msg->data_len_bits + bit_msg->cargo.raw_start_index;
  RETURN_IF_ERROR_PRESENT(ErrorDetection_AddDetection(bit_msg, cfg, false));
}

// JANUS class user id 11 and application type 1 allows the sending and
// receiving of text messages with variable coding and encryption
void addJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
{
  RETURN_IF_ERROR_PRESENT(addCodedEncryptedData(msg->preamble.coding.value, msg->preamble.encryption.value, bit_msg, msg));

  uint16_t error_detection_bits;
  ErrorDetection_CheckLength(&error_detection_bits, cfg->cargo_validation);
  bit_msg->data_len_bits = bit_msg->cargo.raw_len - error_detection_bits;
  // Skip any padding bits
  bit_msg->bit_count = bit_msg->data_len_bits + bit_msg->cargo.raw_start_index;
  RETURN_IF_ERROR_PRESENT(ErrorDetection_AddDetection(bit_msg, cfg, false));
}

// Encryption not yet implemented
void addCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg)
{
  if (encryption != ENCRYPTION_NONE) 
    REGISTER_ERROR(ERROR_SEND_UNKNOWN_JANUS);

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
        if (msg->data[i] > 127) REGISTER_ERROR(ERROR_INVALID_CHARACTER);
        coded_value = msg->data[i];
        coded_len = 7;
        break;
      case CODING_ASCII6:
        RETURN_IF_ERROR_PRESENT(encodeAsAscii6(toupper(msg->data[i]), &coded_value));
        coded_len = 6;
        break;
      default:
        REGISTER_ERROR(ERROR_UNHANDLED_CASE);
        return;
    }
    for (uint16_t j = 0; j < coded_len; j++) {
      bool bit = (coded_value & (1 << (coded_len - 1 - j))) != 0;
      if (Packet_AddBit(bit_msg, bit) == false)
        REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
    }
  }
}

void extractCustomCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  switch (msg->preamble.message_type.value) {
    case INTEGER:
    case STRING:
    case FLOAT:
    case BITS:
    case UNKNOWN:
      extractDataCustomCargo(bit_msg, msg);
      break;
    case EVAL:
      RETURN_IF_ERROR_PRESENT(Evaluate_CodedBer(&msg->eval_info, bit_msg));
      break;
    case RANGING_REQUEST:
    case RANGING_RESPONSE:
      // No cargo
      break;
    default:
      REGISTER_ERROR(ERROR_UNKNOWN_MESSAGE);
  }
}

void extractDataCustomCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  // data_len_bytes is restricted to be a multiple of 8
  uint16_t len_bytes = bit_msg->data_len_bits / 8;

  uint16_t start_position = bit_msg->cargo.raw_start_index;

  for (uint16_t i = 0; i < len_bytes; i++) {
    if (Packet_Get8(bit_msg, &start_position, msg->data + i) == false)
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
  }
}

void extractDataJanusCargo(BitMessage_t* bit_msg, Message_t* msg)
{
  switch (msg->janus_data_type) {
    case JANUS_011_01_SMS:
      msg->uncoded_data_len = Cargo_RawUncodedLength(bit_msg->data_len_bits, msg->preamble.coding.value);
      RETURN_IF_ERROR_PRESENT(extractJanus_11_01_Cargo(bit_msg, msg));
      break;
    default:
      REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
  }
}

void extractJanus_11_01_Cargo(BitMessage_t* bit_msg, Message_t* msg)
{
  extractCodedEncryptedData(msg->preamble.coding.value, msg->preamble.encryption.value, bit_msg, msg);
}

void extractCodedEncryptedData(CodingInfo_t coding, EncryptionInfo_t encryption, BitMessage_t* bit_msg, Message_t* msg)
{
  if (encryption != ENCRYPTION_NONE) REGISTER_ERROR(ERROR_UNKNOWN_JANUS);

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
      REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
      return;
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
      if (Packet_GetBit(bit_msg, bit_msg_bit_index, &bit) == false)
        REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
      
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
        RETURN_IF_ERROR_PRESENT(decodeAscii6(coded_byte, &uncoded_byte));
        break;
      default:
        REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
    }
    if (msg_byte_index >= (PACKET_DATA_MAX_LENGTH_BYTES - 1)) 
      REGISTER_ERROR(ERROR_UNKNOWN_JANUS);
    
    msg->data[msg_byte_index++] = uncoded_byte;
    remaining_length -= chunk_size;
  }
}

void encodeAsAscii6(uint8_t ascii_8, uint8_t* ascii_6)
{
  for (uint8_t i = 0; i < sizeof(ais_6_ascii8_lut) / sizeof(ais_6_ascii8_lut[0]); i++) {
    if (ascii_8 == ais_6_ascii8_lut[i]) {
      *ascii_6 = i;
      return;
    }
  }
  REGISTER_ERROR(ERROR_INVALID_CHARACTER);
}

void decodeAscii6(uint8_t ascii_6, uint8_t* ascii_8)
{
  if (ascii_6 > (sizeof(ais_6_ascii8_lut) / sizeof(ais_6_ascii8_lut[0]))) 
    REGISTER_ERROR(ERROR_INVALID_CHARACTER);
  
  *ascii_8 = ais_6_ascii8_lut[ascii_6];
}
