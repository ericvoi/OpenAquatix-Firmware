/*
 * comm_txrx_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_function_loops.h"

#include "cfg_parameters.h"

#include "mess_main.h"
#include "mess_packet.h"

#include "cmsis_os.h"

#include "check_inputs.h"
#include "number_utils.h"
#include "usb_comm.h"
#include "error_manager.h"
#include "main.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void transmitBitsOut(FunctionContext_t* context);
void transmitBitsFb(FunctionContext_t* context);
void transmitStringOut(FunctionContext_t* context);
void transmitStringFb(FunctionContext_t* context);
void transmitIntOut(FunctionContext_t* context);
void transmitIntFb(FunctionContext_t* context);
void transmitFloatOut(FunctionContext_t* context);
void transmitFloatFb(FunctionContext_t* context);
void rangingRequestTransducer(FunctionContext_t* context);
void rangingRequestFeedback(FunctionContext_t* context);
void togglePrint(FunctionContext_t* context);

void transmitBits(FunctionContext_t* context, bool is_feedback);
void transmitString(FunctionContext_t* context, bool is_feedback);
void transmitInt(FunctionContext_t* context, bool is_feedback);
void transmitFloat(FunctionContext_t* context, bool is_feedback);
void transmitRangingRequest(FunctionContext_t* context, bool is_feedback);

bool parseHexString(FunctionContext_t* context, uint16_t* num_bytes, uint8_t* decoded_bytes);
void sendMessageToTxQueue(FunctionContext_t* context, Message_t* msg, bool is_feedback);

bool inCustomMode(FunctionContext_t* context);

/* Private variables ---------------------------------------------------------*/

extern osMessageQueueId_t regular_tx_queue;

static MenuID_t txrx_menu_children[] = {
  MENU_ID_TXRX_BITSOUT,   MENU_ID_TXRX_BITSFB,    MENU_ID_TXRX_STROUT, 
  MENU_ID_TXRX_STRFB,     MENU_ID_TXRX_INTOUT,    MENU_ID_TXRX_INTFB,
  MENU_ID_TXRX_FLOATOUT,  MENU_ID_TXRX_FLOATFB ,  MENU_ID_TXRX_RANGEOUT,
  MENU_ID_TXRX_RANGEFB,   MENU_ID_TXRX_ENPNT
};
static const MenuNode_t txrx_menu = {
  .id = MENU_ID_TXRX,
  .description = "Transmit and Receive Data Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = txrx_menu_children,
  .num_children = sizeof(txrx_menu_children) / sizeof(txrx_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t txrx_bits_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_BITSOUT
};
static const MenuNode_t txrx_bits_transducer = {
  .id = MENU_ID_TXRX_BITSOUT,
  .description = "Send bits through transducer",
  .handler = transmitBitsOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_bits_transducer_param
};

static ParamContext_t txrx_bits_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_BITSFB
};
static const MenuNode_t txrx_bits_feedback = {
  .id = MENU_ID_TXRX_BITSFB,
  .description = "Send bits through feedback",
  .handler = transmitBitsFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_bits_feedback_param
};

static ParamContext_t txrx_str_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_STROUT
};
static const MenuNode_t txrx_str_transducer = {
  .id = MENU_ID_TXRX_STROUT,
  .description = "Send string through transducer",
  .handler = transmitStringOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_str_transducer_param
};

static ParamContext_t txrx_str_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_STRFB
};
static const MenuNode_t txrx_str_feedback = {
  .id = MENU_ID_TXRX_STRFB,
  .description = "Send string through feedback",
  .handler = transmitStringFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_str_feedback_param
};

static ParamContext_t txrx_int_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_INTOUT
};
static const MenuNode_t txrx_int_transducer = {
  .id = MENU_ID_TXRX_INTOUT,
  .description = "Send integer through transducer",
  .handler = transmitIntOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_int_transducer_param
};

static ParamContext_t txrx_int_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_INTFB
};
static const MenuNode_t txrx_int_feedback = {
  .id = MENU_ID_TXRX_INTFB,
  .description = "Send integer through feedback",
  .handler = transmitIntFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_int_feedback_param
};

static ParamContext_t txrx_float_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_FLOATOUT
};
static const MenuNode_t txrx_float_transducer = {
  .id = MENU_ID_TXRX_FLOATOUT,
  .description = "Send float through transducer",
  .handler = transmitFloatOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_float_transducer_param
};

static ParamContext_t txrx_float_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_FLOATFB
};
static const MenuNode_t txrx_float_feedback = {
  .id = MENU_ID_TXRX_FLOATFB,
  .description = "Send float through feedback",
  .handler = transmitFloatFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_float_feedback_param
};

static ParamContext_t txrx_range_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_RANGEOUT
};
static const MenuNode_t txrx_range_transducer = {
  .id = MENU_ID_TXRX_RANGEOUT,
  .description = "Send ranging request through transducer",
  .handler = rangingRequestTransducer,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_range_transducer_param
};

static ParamContext_t txrx_range_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_RANGEFB
};
static const MenuNode_t txrx_range_feedback = {
  .id = MENU_ID_TXRX_RANGEFB,
  .description = "Send ranging request through feedback",
  .handler = rangingRequestFeedback,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_range_feedback_param
};

static ParamContext_t txrx_toggle_print_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_ENPNT
};
static const MenuNode_t txrx_toggle_print = {
  .id = MENU_ID_TXRX_ENPNT,
  .description = "Toggle printing of received messages",
  .handler = togglePrint,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrx_toggle_print_param
};


/* Exported function definitions ---------------------------------------------*/

void COMM_RegisterTxRxMenu()
{
  bool ret = MenuSystem_RegisterMenu(&txrx_menu) && MenuSystem_RegisterMenu(&txrx_bits_transducer) &&
             MenuSystem_RegisterMenu(&txrx_str_transducer) && MenuSystem_RegisterMenu(&txrx_int_transducer) &&
             MenuSystem_RegisterMenu(&txrx_float_transducer) && MenuSystem_RegisterMenu(&txrx_toggle_print) &&
             MenuSystem_RegisterMenu(&txrx_str_feedback) && MenuSystem_RegisterMenu(&txrx_bits_feedback) &&
             MenuSystem_RegisterMenu(&txrx_int_feedback) && MenuSystem_RegisterMenu(&txrx_float_feedback) &&
             MenuSystem_RegisterMenu(&txrx_range_transducer) && MenuSystem_RegisterMenu(&txrx_range_feedback);
  
  if (ret == false) REGISTER_ERROR(ERROR_MENU_REGISTRATION);
}

/* Private function definitions ----------------------------------------------*/

void transmitBitsOut(FunctionContext_t* context)
{
  transmitBits(context, false);
}
 
void transmitBitsFb(FunctionContext_t* context) 
{
  transmitBits(context, true);
}

void transmitStringOut(FunctionContext_t* context)
{
  transmitString(context, false);
}

void transmitStringFb(FunctionContext_t* context)
{
  transmitString(context, true);
}

void transmitIntOut(FunctionContext_t* context)
{
  transmitInt(context, false);
}

void transmitIntFb(FunctionContext_t* context)
{
  transmitInt(context, true);
}

void transmitFloatOut(FunctionContext_t* context)
{
  transmitFloat(context, false);
}

void transmitFloatFb(FunctionContext_t* context)
{
  transmitFloat(context, true);
}

void rangingRequestTransducer(FunctionContext_t* context)
{
  osEventFlagsSet(print_event_handle, MESS_REQUEST_RANGE_TRANSDUCER);
  context->state->state = PARAM_STATE_COMPLETE;
}

void rangingRequestFeedback(FunctionContext_t* context)
{
  osEventFlagsSet(print_event_handle, MESS_REQUEST_RANGE_FEEDBACK);
  context->state->state = PARAM_STATE_COMPLETE;
}

void togglePrint(FunctionContext_t* context)
{
  COMMLoops_LoopToggle(context, PARAM_PRINT_ENABLED);
}

void transmitBits(FunctionContext_t* context, bool is_feedback)
{
  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter up to %u "
            "bytes in binary data in hexademical to send to the %s with the "
            "format 'F6 1D'...\r\n"
            "Note: The number of bytes must be a power of 2\r\n",
            PACKET_DATA_MAX_LENGTH_BYTES,
            is_feedback ? "feedback network" : "transducer");
        COMM_TransmitData(context->output_buffer, CALC_LEN, 
            context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        uint8_t binary_data[PACKET_DATA_MAX_LENGTH_BYTES];
        uint16_t num_bytes;
        if (parseHexString(context, &num_bytes, binary_data) == false) {
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = is_feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = BITS;
          msg.preamble.message_type.value = BITS;
          msg.preamble.message_type.valid = true;
          msg.length_bits = num_bytes * 8;
          memcpy(msg.data, binary_data, num_bytes);
          msg.delay = false;

          sendMessageToTxQueue(context, &msg, is_feedback);
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void transmitString(FunctionContext_t* context, bool is_feedback)
{
  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a string to "
            "send to the %s with a maximum length of %u characters:\r\n", 
            is_feedback ? "feedback network" : "transducer",
            PACKET_DATA_MAX_LENGTH_BYTES);
        COMM_TransmitData(context->output_buffer, CALC_LEN, 
            context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        if (context->input_len > 128) {
          sprintf((char*) context->output_buffer, "\r\nInput string must be"
              "less than %u characters!\r\n", PACKET_DATA_MAX_LENGTH_BYTES);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = is_feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = STRING;
          msg.length_bits = 8 * context->input_len;
          msg.preamble.message_type.value = STRING;
          msg.preamble.message_type.valid = true;
          for (uint16_t i = 0; i < msg.length_bits / 8; i++) {
            if (context->input_len > i) {
              msg.data[i] = context->input[i];
            }
            else {
              msg.data[i] = '\0';
            }
          }
          msg.delay = false;
          
          sendMessageToTxQueue(context, &msg, is_feedback);
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void transmitInt(FunctionContext_t* context, bool is_feedback)
{
  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter an integer to "
            "send to the %s between 0 and 4,294,967,295:\r\n", 
            is_feedback ? "feedback network" : "transducer");
        COMM_TransmitData(context->output_buffer, CALC_LEN, 
            context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        uint32_t input;
        if (checkUint32(context->input, context->input_len, &input, 0, 4294967295) == false) {
          sprintf((char*) context->output_buffer, "\r\nInvalid input!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = is_feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = INTEGER;
          msg.preamble.message_type.value = INTEGER;
          msg.preamble.message_type.valid = true;
          msg.length_bits = 8 * sizeof(uint32_t);
          memcpy(&msg.data[0], &input, sizeof(uint32_t));
          msg.delay = false;
          
          sendMessageToTxQueue(context, &msg, is_feedback);
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void transmitFloat(FunctionContext_t* context, bool is_feedback)
{
  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a float to "
            "send to the %s:\r\n", 
            is_feedback ? "feedback network" : "transducer");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        float input;
        if (checkFloat(context->input, &input, -1e30f, 1e30f) == false) {
          sprintf((char*) context->output_buffer, "\r\nInvalid input!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = is_feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = FLOAT;
          msg.preamble.message_type.value = FLOAT;
          msg.preamble.message_type.valid = true;
          msg.length_bits = 8 * sizeof(float);
          memcpy(&msg.data[0], &input, sizeof(float));
          msg.delay = false;

          sendMessageToTxQueue(context, &msg, is_feedback);
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

bool parseHexString(FunctionContext_t* context, uint16_t* num_bytes, uint8_t* decoded_bytes)
{
  if (context == NULL || num_bytes == NULL) {
    COMM_TransmitData("\r\nInternal Error!\r\n", CALC_LEN, 
        context->comm_interface);
    return false;
  }

  const char* ptr = context->input;
  uint16_t ptr_index = 0;
  *num_bytes = 0;
  bool high_digit = true;
  uint8_t current_byte;

  // skip preceding whitespace
  while (ptr[ptr_index] != '\0' && isspace((unsigned char) ptr[ptr_index])) ptr_index++;

  // skip 0x if present
  if (ptr[ptr_index] == '0' && 
      (ptr[ptr_index + 1] == 'x' || ptr[ptr_index + 1] == 'X')) {

    ptr_index += 2;
  }

  while (ptr[ptr_index] != '\0' && *num_bytes < PACKET_DATA_MAX_LENGTH_BYTES) {

    if (ptr_index >= context->input_len || ptr[ptr_index] == '\0') break;

    // Common delimiters to skip
    if (isspace((unsigned char) ptr[ptr_index]) || ptr[ptr_index] == ',' || ptr[ptr_index] == ':') {
      ptr_index++;
      continue;
    }

    uint8_t nibble;
    if (ptr[ptr_index] >= '0' && ptr[ptr_index] <= '9') {
      nibble = ptr[ptr_index] - '0';
    }
    else if (ptr[ptr_index] >= 'a' && ptr[ptr_index] <= 'f') {
      nibble = ptr[ptr_index] - 'a' + 10;
    }
    else if (ptr[ptr_index] >= 'A' && ptr[ptr_index] <= 'F') {
      nibble = ptr[ptr_index] - 'A' + 10;
    } else {
      COMM_TransmitData("\r\nError: Unknown character detected\r\n", CALC_LEN,
          context->comm_interface);
      return false;
    }

    if (high_digit == true) {
      current_byte = nibble << 4;
      high_digit = false;
    } 
    else {
      current_byte |= nibble;
      decoded_bytes[(*num_bytes)++] = current_byte;
      high_digit = true;
    } 

    ptr_index++;
  }

  if (NumberUtils_IsPowerOf2(*num_bytes) == true && high_digit == true &&
      *num_bytes <= PACKET_DATA_MAX_LENGTH_BYTES) {
    return true;
  }
  else { 
    sprintf((char*) context->output_buffer,"\r\nError: The input length must "
        "be a power of 2 and less than %u. The received input is %u bytes long\r\n",
        PACKET_DATA_MAX_LENGTH_BYTES, *num_bytes);
    COMM_TransmitData(context->output_buffer, CALC_LEN, 
        context->comm_interface);
    return false;
  }
}

void sendMessageToTxQueue(FunctionContext_t* context, Message_t* msg, bool is_feedback)
{
  if (inCustomMode(context) == false) return;

  if (Param_GetUint8(PARAM_ID, (uint8_t*) &msg->preamble.modem_id.value) == false) {
    COMM_TransmitData("\r\nError getting sender ID. Message not sent\r\n", 
        CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }
  msg->preamble.modem_id.valid = true;
  if (osMessageQueuePut(regular_tx_queue, msg, 0, 0) == osOK) {
    sprintf((char*) context->output_buffer, "\r\nSuccessfully added to"
        " %s queue!\r\n\r\n", is_feedback ? "feedback network" : "transducer");
    COMM_TransmitData(context->output_buffer, CALC_LEN, 
        context->comm_interface);
  }
  else {
    sprintf((char*) context->output_buffer, "\r\nError adding message to"
        " %s queue\r\n\r\n", is_feedback ? "feedback network" : "transducer");
    COMM_TransmitData(context->output_buffer, CALC_LEN, 
        context->comm_interface);
  }
  context->state->state = PARAM_STATE_COMPLETE;
}

bool inCustomMode(FunctionContext_t* context)
{
  MessagingProtocol_t protocol;
  if (Param_GetUint8(PARAM_PROTOCOL, &protocol) == false) {
    context->state->state = PARAM_STATE_COMPLETE;
    COMM_TransmitData("Cannot find protocol information. Message not sent.\r\n", CALC_LEN, context->comm_interface);
    return false;
  }

  if (protocol == PROTOCOL_CUSTOM) {
    return true;
  }

  context->state->state = PARAM_STATE_COMPLETE;
  COMM_TransmitData("Cannot send custom messages in non-custom modes. Message not sent\r\n", CALC_LEN, context->comm_interface);
  return false;
}
