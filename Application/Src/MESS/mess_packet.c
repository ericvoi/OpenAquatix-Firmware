/*
 * mess_packet.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_packet.h"
#include "mess_error_correction.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint8_t modem_id = DEFAULT_ID;
static bool is_stationary = DEFAULT_STATIONARY_FLAG;

/* Private function prototypes -----------------------------------------------*/

bool addPreamble(BitMessage_t* bit_msg, Message_t* msg);
bool addMessage(BitMessage_t* bit_msg, Message_t* msg);
void initPacket(BitMessage_t* bit_msg);
bool addChunk(BitMessage_t* bit_msg, uint8_t chunk, uint8_t chunk_size);
bool addData(BitMessage_t* bit_msg, void* data, uint8_t num_bits);
bool getData(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t num_bits, void* data);

/* Exported function definitions ---------------------------------------------*/

bool Packet_PrepareTx(Message_t* msg, BitMessage_t* bit_msg)
{
  if (msg == NULL || bit_msg == NULL) {
    return false;
  }

  initPacket(bit_msg);

  // Add preamble to bit packet
  if (msg->data_type != EVAL) {
    if (addPreamble(bit_msg, msg) == false) {
      return false;
    }
  }

  // Adds the data payload bits to the packet
  if (addMessage(bit_msg, msg) == false) {
    return false;
  }

  // Add the error correction bits as specified by the user
  if (msg->data_type != EVAL) {
    if (ErrorCorrection_AddCorrection(bit_msg) == false) {
      return false;
    }
  }
  return true;
}

bool Packet_PrepareRx(BitMessage_t* bit_msg)
{
  initPacket(bit_msg);

  return true;
}

bool Packet_AddBit(BitMessage_t* bit_msg, bool bit)
{
  if (bit_msg->bit_count >= PACKET_MAX_LENGTH_BITS) {
    return false;
  }

  uint8_t byte_index = bit_msg->bit_count / 8;
  uint8_t bit_position = bit_msg-> bit_count % 8;

  if (bit == true) {
    bit_msg->data[byte_index] |= (1 << (7 - bit_position));
  } else {
    bit_msg->data[byte_index] &= ~(1 << (7 - bit_position));
  }

  bit_msg->bit_count++;
  return true;
}

bool Packet_GetBit(BitMessage_t* bit_msg, uint16_t position, bool* bit)
{
  if (position >= bit_msg->bit_count) {
    return false;
  }

  uint16_t byte_index = position / 8;
  uint8_t bit_position = position % 8;

  *bit = (bit_msg->data[byte_index] & (1 << (7 - bit_position))) != 0;

  return true;
}

bool Packet_Get8BitChunk(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t chunk_length, uint8_t* ret)
{
  if (*start_position + chunk_length > bit_msg->bit_count) {
    return false;
  }
  uint8_t start_byte = *start_position / 8;
  uint16_t raw_chunk;
  memcpy(&raw_chunk, &bit_msg->data[start_byte], sizeof(uint16_t));

  raw_chunk = (raw_chunk << 8) | (raw_chunk >> 8);

  uint8_t start_ignore_bits = *start_position % 8;
  uint8_t end_ignore_bits = 16 - chunk_length;

  raw_chunk = raw_chunk << start_ignore_bits;
  raw_chunk = raw_chunk >> end_ignore_bits;

  *ret = (uint8_t) raw_chunk;
  *start_position += chunk_length;
  return true;
}

bool Packet_Add8(BitMessage_t* bit_msg, uint8_t data)
{
  return addData(bit_msg, &data, 8 * sizeof(uint8_t));
}

bool Packet_Add16(BitMessage_t* bit_msg, uint16_t data)
{
 return addData(bit_msg, &data, 8 * sizeof(uint16_t));
}

bool Packet_Add32(BitMessage_t* bit_msg, uint32_t data)
{
  return addData(bit_msg, &data, 8 * sizeof(uint32_t));
}

bool Packet_Get8(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t* data)
{
  return getData(bit_msg, start_position, 8 * sizeof(uint8_t), data);
}

bool Packet_Get16(BitMessage_t* bit_msg, uint16_t* start_position, uint16_t* data)
{
  return getData(bit_msg, start_position, 8 * sizeof(uint16_t), data);
}

bool Packet_Get32(BitMessage_t* bit_msg, uint16_t* start_position, uint32_t* data)
{
  return getData(bit_msg, start_position, 8 * sizeof(uint32_t), data);
}

uint16_t Packet_MinimumSize(uint16_t str_len)
{
  size_t packet_size = 1;

  // Keep doubling the packet size until it's large enough
  while (packet_size < str_len) {
      packet_size *= 2;
  }

  return packet_size;
}

bool Packet_RegisterParams()
{
  uint32_t min_u32 = MIN_ID;
  uint32_t max_u32 = MAX_ID;
  if (Param_Register(PARAM_ID, "the modem identifier", PARAM_TYPE_UINT8,
      &modem_id, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_u32 = MIN_STATIONARY_FLAG;
  max_u32 = MAX_STATIONARY_FLAG;
  if (Param_Register(PARAM_STATIONARY_FLAG, "stationary flag", PARAM_TYPE_UINT8,
      &is_stationary, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  return true;
}

/* Private function definitions ----------------------------------------------*/

void initPacket(BitMessage_t* bit_msg)
{
  memset(bit_msg->data, 0, sizeof(bit_msg->data));
  bit_msg->bit_count = 0;
  bit_msg->sender_id = 255;
  bit_msg->contents_data_type = UNKNOWN;
  bit_msg->final_length = 0;
  bit_msg->stationary_flag = false;
  bit_msg->preamble_received = false;
  bit_msg->fully_received = false;
  bit_msg->added_to_queue = false;
}

bool addPreamble(BitMessage_t* bit_msg, Message_t* msg)
{
  if (msg->length_bits > PACKET_DATA_MAX_LENGTH_BITS) {
    return false;
  }

  if (addChunk(bit_msg, modem_id, PACKET_SENDER_ID_BITS) == false) {
    return false;
  }

  if (addChunk(bit_msg, msg->data_type, PACKET_MESSAGE_TYPE_BITS) == false) {
    return false;
  }

  uint8_t length_index = 0;
  uint16_t length_accomodated = 8;

  while (length_accomodated < msg->length_bits) {
    length_index++;
    length_accomodated = length_accomodated << 1;
  }

  if (addChunk(bit_msg, length_index, PACKET_LENGTH_BITS) == false) {
    return false;
  }

  bit_msg->final_length += length_accomodated;
  uint16_t error_length;
  if (ErrorCorrection_CheckLength(&error_length) == false) {
    return false;
  }
  bit_msg->final_length += error_length;

  if (Packet_AddBit(bit_msg, is_stationary) == false) {
    return false;
  }

  bit_msg->final_length += PACKET_PREAMBLE_LENGTH_BITS;

  return true;
}

bool addMessage(BitMessage_t* bit_msg, Message_t* msg)
{
  if (msg->data_type == BITS) {
    return false;
  }

  for (uint16_t i = 0; i < msg->length_bits; i++) {
    uint16_t byte_index = i / 8;
    uint16_t bit_index = i % 8;
    bool bit = (msg->data[byte_index] & (1 << (7 - bit_index))) != 0;
    if (Packet_AddBit(bit_msg, bit) == false)
      return false;
  }
  return true;
}

bool addChunk(BitMessage_t* bit_msg, uint8_t chunk, uint8_t chunk_size)
{
  if (chunk_size > 8) {
    return false;
  }

  for (uint8_t i = 8 - chunk_size; i < 8; i++) {
    bool bit = (chunk & (1 << (7 - i))) != 0;
    if (Packet_AddBit(bit_msg, bit) == false) {
      return false;
    }
  }
  return true;
}

bool addData(BitMessage_t* bit_msg, void* data, uint8_t num_bits)
{
  if (bit_msg == NULL || data == NULL || num_bits > 32) {
    return false;
  }

  if ((num_bits % 8) != 0) {
    return false;
  }

  for (uint8_t i = 0; i < num_bits; i++) {
    uint8_t byte_index = i / 8;
    uint8_t bit_index = i % 8;
    bool bit = (*(((uint8_t*) data) + byte_index) & (1 << (7 - bit_index))) != 0;
    if (Packet_AddBit(bit_msg, bit) == false) {
      return false;
    }
  }
  return true;
}

bool getData(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t num_bits, void* data)
{
  if (bit_msg == NULL || start_position == NULL || data == NULL || num_bits > 32) {
    return false;
  }

  if ((num_bits % 8) != 0) {
    return false;
  }

  for (uint8_t i = 0; i < num_bits; i++) {
    uint8_t byte_index = i / 8;
    uint8_t bit_index = i % 8;
    bool bit;
    if (Packet_GetBit(bit_msg, *start_position + i, &bit) == false) {
      return false;
    }
    if (bit == true) {
      *(((uint8_t*) data) + byte_index) |= (1 << (7 - bit_index));
    } else {
      *(((uint8_t*) data) + byte_index) &= ~(1 << (7 - bit_index));
    }
  }
  *start_position += num_bits;
  return true;
}
// TODO: Add the error correction codes to the messages. Implement error correction functions to find crcs and checksums
