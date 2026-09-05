/*
 * janus_utils.c
 *
 *  Created on: Jul 16, 2026
 *      Author: ericv
 * 
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "janus_utils.h"
#include "error_manager.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define ENCODED_CARGO_BITS          7

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

uint16_t JanusUtil_EncodeResTime(uint32_t time_ms)
{
  uint16_t reservation_time_index = (uint16_t) ceilf(time_ms * 25.0f / 4.0f + 14.0f);

  uint8_t e = 0;
  while ((15 + 16) * (1 << e) < reservation_time_index) {
    e++;
    if (e > 7) REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);
  }
  uint8_t m = (uint8_t) ceil(((float) reservation_time_index) / ((float) (1 << e)) - 16.0f);
  if (m > 15)
    REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);
  return ((e & 0x07) << 4) | (m & 0x0F);
}

uint32_t JanusUtil_DecodeResTime(uint16_t coded_value)
{
  if (coded_value >= (1 << JANUS_RESERVATION_BITS))
    REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);

  uint8_t m = coded_value & 0x0F;
  uint8_t e = (coded_value >> 4) & 0x07;
  float reservation_time = ((m + 16.0f) * (1 << e) - 14.0f) * (4.0f / 25.0f);
  return (uint32_t) (reservation_time * 1000.0f);
}

uint16_t JanusUtil_EncodeLen(uint16_t n_bits,  uint8_t n_e, uint8_t n_x)
{
  if (n_bits == 0) 
    REGISTER_ERROR_NON_VOID(ERROR_INVALID_CARGO_LENGTH, 0);
  if ((n_e + n_x) != ENCODED_CARGO_BITS)
    REGISTER_ERROR_NON_VOID(ERROR_JANUS_ENCODED_LEN, 0);

  uint16_t num_bytes = (n_bits + 7) / 8;
  uint8_t e_amt = ENCODED_CARGO_BITS - n_x;
  uint8_t x_amt = ENCODED_CARGO_BITS - n_e;

  uint8_t e = 0;
  uint16_t offset = (1 << x_amt) << 0;
  // find the minimum exponent needed to encode the data
  while (offset < num_bytes) {
    e++;
    offset += (1 << x_amt) << e;
    if (e >= (1 << e_amt))
      REGISTER_ERROR_NON_VOID(ERROR_JANUS_ENCODED_LEN, 0);
  }

  offset -= (1 << x_amt) << e;

  uint16_t remaining_bytes = num_bytes - offset;
  uint16_t granularity = (1 << e);
  // Round up to the best nearest x
  uint8_t x = (remaining_bytes + granularity - 1) / granularity - 1;
  if (x >= (1 << x_amt))
    REGISTER_ERROR_NON_VOID(ERROR_JANUS_ENCODED_LEN, 0);
  uint16_t encoded_len = 0;
  encoded_len |= e << x_amt;
  encoded_len |= x;

  return encoded_len;
}

uint16_t JanusUtil_DecodeLen(uint16_t encoded, uint8_t n_e, uint8_t n_x)
{
  if ((n_e + n_x) != ENCODED_CARGO_BITS)
    REGISTER_ERROR_NON_VOID(ERROR_JANUS_ENCODED_LEN, 0);
  if (encoded >= (1 << ENCODED_CARGO_BITS))
    REGISTER_ERROR_NON_VOID(ERROR_JANUS_ENCODED_LEN, 0);
  // Number of bits corresponding to e and x
  uint8_t e_amt = ENCODED_CARGO_BITS - n_x;
  uint8_t x_amt = ENCODED_CARGO_BITS - n_e;
  uint16_t e = (encoded >> x_amt) & ((1 << e_amt) - 1); // Extract bits n_x:(ENCODED_CARGO_BITS - 1)
  uint16_t x = encoded & ((1 << x_amt) - 1); // Extract bits 0:(n_x - 1)

  uint16_t num_bytes = (1 << e) * (x + 1);
  // add the offset
  for (uint8_t i = 0; i < e; i++) {
    num_bytes += 1 << (x_amt + i);
  }
  return num_bytes * 8;
}

/* Private function definitions ----------------------------------------------*/
