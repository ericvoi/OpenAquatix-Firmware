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
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Cargo_Add(BitMessage_t* bit_msg, Message_t* msg, const DspConfig_t* cfg)
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
  bit_msg->cargo.ecc_len = ErrorCorrection_GetLength(bit_msg->cargo.raw_len, 
                                                     cfg->cargo_ecc_method);
  bit_msg->combined_message_len = bit_msg->cargo.raw_len + bit_msg->preamble.raw_len;
  if (msg->data_type != EVAL) {
    if (ErrorDetection_AddDetection(bit_msg, cfg, false) == false) {
      return false;
    }
  }
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
  (void)(cfg);
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

/* Private function definitions ----------------------------------------------*/
