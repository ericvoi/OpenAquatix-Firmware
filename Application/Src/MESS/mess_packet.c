/*
 * mess_packet.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include <mess_error_detection.h>
#include "mess_packet.h"
#include "mess_sync.h"
#include "mess_preamble.h"
#include "mess_cargo.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

bool initPacket(BitMessage_t* bit_msg, const DspConfig_t* cfg);
bool addChunk(BitMessage_t* bit_msg, uint8_t chunk, uint8_t chunk_size);
bool addData(BitMessage_t* bit_msg, void* data, uint8_t num_bits);
bool getData(BitMessage_t* bit_msg, uint16_t* start_position, uint8_t num_bits, void* data);

/* Exported function definitions ---------------------------------------------*/

bool Packet_PrepareTx(Message_t* msg, BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (msg == NULL || bit_msg == NULL) {
    return false;
  }

  if (initPacket(bit_msg, cfg) == false) {
    return false;
  }

  // Add preamble to bit packet
  if (Preamble_Add(bit_msg, msg, cfg) == false) {
    return false;
  }

  // Adds the data payload bits to the packet
  if (Cargo_Add(bit_msg, msg, cfg) == false) {
    return false;
  }
  return true;
}

bool Packet_PrepareRx(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (initPacket(bit_msg, cfg) == false) {
    return false;
  }

  return true;
}

bool Packet_AddBit(BitMessage_t* bit_msg, bool bit)
{
  if (Packet_SetBit(bit_msg, bit_msg->bit_count, bit) == false) {
    return false;
  }

  bit_msg->bit_count++;
  return true;
}

bool Packet_GetBit(const BitMessage_t* bit_msg, uint16_t position, bool* bit)
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

bool Packet_GetChunk(BitMessage_t* bit_msg, uint16_t start_position, uint8_t num_bits, uint16_t* data)
{
  if (num_bits > 8 * sizeof(uint16_t) || bit_msg == NULL || data == NULL) {
    return false;
  }

  return getData(bit_msg, &start_position, num_bits, data);
}

bool Packet_AddChunk(BitMessage_t* bit_msg, uint8_t num_bits, uint16_t data)
{
  if (num_bits > 8 * sizeof(uint16_t) || bit_msg == NULL) {
    return false;
  }

  return addData(bit_msg, &data, num_bits);
}

bool Packet_FlipBit(BitMessage_t* bit_msg, uint16_t bit_index)
{
  if (bit_msg == NULL) {
    return false;
  }
  if (bit_index >= bit_msg->final_length) {
    return false;
  }

  uint16_t byte_index = bit_index / 8;
  uint8_t bit_position = bit_index % 8;

  uint8_t mask = 1 << (7 - bit_position);

  bit_msg->data[byte_index] ^= mask;
  return true;
}

bool Packet_SetBit(BitMessage_t* bit_msg, uint16_t bit_index, bool bit)
{
  if (bit_index >= PACKET_MAX_LENGTH_BYTES * 8) {
    return false;
  }

  uint16_t byte_index = bit_index / 8;
  uint8_t bit_position = bit_index % 8;

  if (bit == true) {
    bit_msg->data[byte_index] |= (1 << (7 - bit_position));
  } else {
    bit_msg->data[byte_index] &= ~(1 << (7 - bit_position));
  }
  return true;
}

bool Packet_Compare(const BitMessage_t* msg1, const BitMessage_t* msg2, bool* identical)
{
  if (msg1 == NULL || msg2 == NULL || identical == NULL) {
    return false;
  }

  uint16_t bit_count1 = msg1->combined_message_len;
  uint16_t bit_count2 = msg2->combined_message_len;

  if (bit_count1 != bit_count2) {
    *identical = false;
    return true;
  }

  uint16_t byte_count = bit_count1 / 8;
  uint8_t remaining_bits = bit_count1 % 8;

  if (memcmp(msg1->data, msg2->data, byte_count) == 0) {
    *identical = true;
  }
  else {
    *identical = false;
    return true;
  }

  uint8_t last_byte1 = msg1->data[byte_count] >> (8 - remaining_bits);
  uint8_t last_byte2 = msg2->data[byte_count] >> (8 - remaining_bits);

  *identical = last_byte1 == last_byte2;
  return true;
}

/**
 * The number of bytes in a amessage can be encoded according to JANUS with
 * 7 bits [e e x x x x x], e=exponent, x=value + offset(e). The offset is
 * simply the sum from N=0 to e-1 of 2^(5+N). See ANEP-87 cargo length for
 * further details 
 */
bool Packet_MinimumLengthIndex(uint16_t num_bytes, uint8_t* length_index)
{
  if (num_bytes > 480 || length_index == NULL) {
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
  *length_index = 0;
  *length_index |= ((e & 0x03) << 5);
  *length_index |= x & 0x1F;
  return true;
}

bool Packet_CargoBytes(uint8_t length_index, uint16_t* num_bytes)
{
  if (length_index > 127 || num_bytes == NULL) {
    return false;
  }

  uint16_t e = (length_index >> 5) & 0x03;
  uint16_t x = length_index & 0x1F;

  *num_bytes = (1 << e) * (x + 1);
  // add the offset
  for (uint8_t i = 0; i < e; i++) {
    *num_bytes += ((1 << 5) << i);
  }
  return true;
}

bool Packet_Copy(const BitMessage_t* src_msg, BitMessage_t* dest_msg, const uint16_t start_index, const uint16_t length)
{
  for (uint16_t i = start_index; i < start_index + length; i++) {
    bool bit;
    if (Packet_GetBit(src_msg, i, &bit) == false) {
      return false;
    }
    if (Packet_SetBit(dest_msg, i, bit) == false) {
      return false;
    }
  }
  return true;
}

bool Packet_RegisterParams()
{
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool initPacket(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  memset(bit_msg->data, 0, sizeof(bit_msg->data));
  bit_msg->bit_count = 0;
  bit_msg->contents_data_type = UNKNOWN;
  bit_msg->final_length = 0;

  if (Preamble_UpdateNumBits(bit_msg, cfg) == false) {
    return false;
  }

  // Cant calculate lengths without decoding the preamble first
  bit_msg->cargo.raw_len = 0;
  bit_msg->cargo.ecc_len = 0;
  bit_msg->cargo.raw_start_index = bit_msg->preamble.raw_start_index + bit_msg->preamble.raw_len;
  bit_msg->cargo.ecc_start_index = bit_msg->preamble.ecc_start_index + bit_msg->preamble.ecc_len;

  bit_msg->preamble_received = false;
  bit_msg->fully_received = false;
  bit_msg->added_to_queue = false;
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
