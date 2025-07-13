/*
 * mess_evaluate.c
 *
 *  Created on: Mar 16, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_evaluate.h"
#include "mess_main.h"
#include "mess_packet.h"

#include "cfg_parameters.h"
#include "cfg_defaults.h"

#include "prbs.h"

#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t eval_message_length = DEFAULT_EVAL_MESSAGE_LEN;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Evaluate_AddCargo(BitMessage_t* bit_msg)
{
  PRBS_Reset();
  for (uint16_t i = 0; i < bit_msg->cargo.raw_len; i++) {
    bool new_bit = PRBS_GetNext();
    if (Packet_AddBit(bit_msg, new_bit) == false) {
      return false;
    }
  }
  bit_msg->data_len_bits = bit_msg->cargo.raw_len;
  return true;
}

bool Evaluate_CodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg)
{
  eval_info->coded_errors = 0;
  PRBS_Reset();
  for (uint16_t i = 0; i < bit_msg->cargo.raw_len; i++) {
    uint16_t bit_index = bit_msg->cargo.raw_start_index + i;
    bool reference_bit = PRBS_GetNext();
    bool received_bit;
    if (Packet_GetBit(bit_msg, bit_index, &received_bit) == false) {
      return false;
    }
    if (reference_bit != received_bit) {
      eval_info->coded_errors++;
    }
  }
  eval_info->coded_bits = bit_msg->cargo.raw_len;
  return true;
}

bool Evaluate_UncodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  eval_info->uncoded_errors = 0;
  BitMessage_t reference_bit_msg;
  Message_t reference_msg;
  reference_msg.length_bits = bit_msg->cargo.raw_len;
  reference_msg.data_type = EVAL;
  reference_msg.preamble.message_type.value = EVAL;
  reference_msg.preamble.message_type.valid = true;
  if (Packet_PrepareTx(&reference_msg, &reference_bit_msg, cfg) == false) {
    return false;
  }
  if (ErrorCorrection_AddCorrection(&reference_bit_msg, cfg) == false) {
    return false;
  }
  for (uint16_t i = 0; i < bit_msg->cargo.ecc_len; i++) {
    uint16_t bit_index = bit_msg->cargo.ecc_start_index + i;
    bool received_bit, reference_bit;
    if (Packet_GetBit(bit_msg, bit_index, &received_bit) == false) {
      return false;
    }
    if (Packet_GetBit(&reference_bit_msg, bit_index, &reference_bit) == false) {
      return false;
    }
    if (received_bit != reference_bit) {
      eval_info->uncoded_errors++;
    }
  }
  eval_info->uncoded_bits = bit_msg->cargo.ecc_len;
  return true;
}

bool Evaluate_RegisterParams()
{
  uint32_t min_u32 = MIN_EVAL_MESSAGE_LEN;
  uint32_t max_u32 = MAX_EVAL_MESSAGE_LEN;
  if (Param_Register(PARAM_EVAL_MESSAGE_LEN, "evaluation message length", PARAM_TYPE_UINT16,
                     &eval_message_length, sizeof(uint16_t),
                     &min_u32, &max_u32, NULL) == false) {
    return false;
  }
  return true;
}

/* Private function definitions ----------------------------------------------*/
