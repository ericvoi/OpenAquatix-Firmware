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

#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define MSG_1_SEQUENCE    0x55        // 01010101
#define MSG_2_SEQUENCE    0xCC        // 11001100
#define MSG_3_SEQUENCE    0x0F        // 00001111
#define MSG_4_SEQUENCE    0x00FF      // 00000000 11111111
#define MSG_5_SEQUENCE    0x399C7A73U // 01110011 00111000 11110100 1110011

/* Private macro -------------------------------------------------------------*/

#define EVAL_MESSAGE_MEMORY_BYTES (EVAL_MESSAGE_LENGTH / 8 + 1)

/* Private variables ---------------------------------------------------------*/

uint8_t msg1[EVAL_MESSAGE_MEMORY_BYTES];
uint8_t msg2[EVAL_MESSAGE_MEMORY_BYTES];
uint8_t msg3[EVAL_MESSAGE_MEMORY_BYTES];
uint8_t msg4[EVAL_MESSAGE_MEMORY_BYTES];
uint8_t msg5[EVAL_MESSAGE_MEMORY_BYTES];


/* Private function prototypes -----------------------------------------------*/

bool getEvaluationMessage(uint8_t** eval_message, uint8_t message_index);

/* Exported function definitions ---------------------------------------------*/

bool Evaluate_Init(void)
{
  memset(msg1, MSG_1_SEQUENCE, EVAL_MESSAGE_MEMORY_BYTES);
  memset(msg2, MSG_2_SEQUENCE, EVAL_MESSAGE_MEMORY_BYTES);
  memset(msg3, MSG_3_SEQUENCE, EVAL_MESSAGE_MEMORY_BYTES);
  /* Unpack multi-byte sequences to individual bytes for msg4 and msg5 */
  for (uint8_t i = 0; i < EVAL_MESSAGE_MEMORY_BYTES; i++) {
    msg4[i] = (uint8_t) (MSG_4_SEQUENCE >> ((1 - (i % sizeof(uint16_t))) * 8));
  }
  for (uint8_t i = 0; i < EVAL_MESSAGE_MEMORY_BYTES; i++) {
    msg5[i] = (uint8_t) (MSG_5_SEQUENCE >> ((3 - (i % sizeof(uint32_t))) * 8));
  }

  return true;
}

bool Evaluate_GetBit(uint8_t message_index, uint16_t bit_index, bool* bit)
{
  if (bit_index >= EVAL_MESSAGE_LENGTH) {
    return false;
  }

  uint16_t byte_index = bit_index / 8;
  uint8_t bit_position = bit_index % 8;

  uint8_t* eval_message;

  if (getEvaluationMessage(&eval_message, message_index) == false) {
    return false;
  }

  *bit = (eval_message[byte_index] & (1 << (7 - bit_position))) != 0;

  return true;
}

bool Evaluate_CalculateBitErrorRate(EvalMessageInfo_t* data, Message_t* msg, uint8_t message_index)
{
  uint16_t errors = 0;

  for (uint16_t i = 0; i < EVAL_MESSAGE_LENGTH; i++) {
    bool truth_bit;
    bool calc_bit;
    if (Evaluate_GetBit(message_index, i, &truth_bit) == false) {
      return false;
    }
    if (Evaluate_GetMessageBit(msg, i, &calc_bit) == false) {
      return false;
    }
    if (truth_bit != calc_bit) {
      errors++;
    }
  }
  data->bit_error_rate = (float) errors / (float) EVAL_MESSAGE_LENGTH;
  return true;
}


bool Evaluate_GetMessageBit(Message_t* msg, uint16_t bit_index, bool* bit)
{
  if (bit_index >= EVAL_MESSAGE_LENGTH) {
    return false;
  }

  uint16_t byte_index = bit_index / 8;
  uint8_t bit_position = bit_index % 8;

  *bit = (msg->data[byte_index] & (1 << (7 - bit_position))) != 0;

  return true;
}

bool Evaluate_CopyEvaluationMessage(Message_t* msg)
{
  uint8_t message_index;
  if (Param_GetUint8(PARAM_EVAL_MESSAGE, &message_index) == false) {
    return false;
  }

  uint8_t* eval_message;

  if (getEvaluationMessage(&eval_message, message_index) == false) {
    return false;
  }

  memcpy(msg->data, eval_message, EVAL_MESSAGE_MEMORY_BYTES);

  msg->length_bits = EVAL_MESSAGE_LENGTH;

  return true;
}

/* Private function definitions ----------------------------------------------*/

bool getEvaluationMessage(uint8_t** eval_message, uint8_t message_index)
{
  switch (message_index) {
    case 1:
      *eval_message = msg1;
      break;
    case 2:
      *eval_message = msg2;
      break;
    case 3:
      *eval_message = msg3;
      break;
    case 4:
      *eval_message = msg4;
      break;
    case 5:
      *eval_message = msg5;
      break;
    default:
      return false;
  }
  return true;
}
