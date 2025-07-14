/*
 * comm_txrx_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void transmitBitsOut(void* argument);
void transmitBitsFb(void* argument);
void transmitStringOut(void* argument);
void transmitStringFb(void* argument);
void transmitIntOut(void* argument);
void transmitIntFb(void* argument);
void transmitFloatOut(void* argument);
void transmitFloatFb(void* argument);
void togglePrint(void* argument);

void transmitBits(FunctionContext_t* context, bool is_feedback);
void transmitString(FunctionContext_t* context, bool is_feedback);
void transmitInt(FunctionContext_t* context, bool is_feedback);
void transmitFloat(FunctionContext_t* context, bool is_feedback);

bool parseHexString(FunctionContext_t* context, uint16_t* num_bytes, uint8_t* decoded_bytes);
void sendMessageToTxQueue(FunctionContext_t* context, Message_t* msg, bool is_feedback);

/* Private variables ---------------------------------------------------------*/

static MenuID_t txrxMenuChildren[] = {
  MENU_ID_TXRX_BITSOUT,   MENU_ID_TXRX_BITSFB,    MENU_ID_TXRX_STROUT, 
  MENU_ID_TXRX_STRFB,     MENU_ID_TXRX_INTOUT,    MENU_ID_TXRX_INTFB,
  MENU_ID_TXRX_FLOATOUT,  MENU_ID_TXRX_FLOATFB ,  MENU_ID_TXRX_ENPNT
};
static const MenuNode_t txrxMenu = {
  .id = MENU_ID_TXRX,
  .description = "Transmit and Receive Data Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = txrxMenuChildren,
  .num_children = sizeof(txrxMenuChildren) / sizeof(txrxMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t txrxBitsTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_BITSOUT
};
static const MenuNode_t txrxBitsTransducer = {
  .id = MENU_ID_TXRX_BITSOUT,
  .description = "Bits Through Transducer",
  .handler = transmitBitsOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxBitsTransducerParam
};

static ParamContext_t txrxBitsFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_BITSFB
};
static const MenuNode_t txrxBitsFeedback = {
  .id = MENU_ID_TXRX_BITSFB,
  .description = "Bits Through Feedback",
  .handler = transmitBitsFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxBitsFeedbackParam
};

static ParamContext_t txrxStrTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_STROUT
};
static const MenuNode_t txrxStrTransducer = {
  .id = MENU_ID_TXRX_STROUT,
  .description = "String Through Transducer",
  .handler = transmitStringOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxStrTransducerParam
};

static ParamContext_t txrxStrFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_STRFB
};
static const MenuNode_t txrxStrFeedback = {
  .id = MENU_ID_TXRX_STRFB,
  .description = "String Through Feedback",
  .handler = transmitStringFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxStrFeedbackParam
};

static ParamContext_t txrxIntTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_INTOUT
};
static const MenuNode_t txrxIntTransducer = {
  .id = MENU_ID_TXRX_INTOUT,
  .description = "Integer Through Transducer",
  .handler = transmitIntOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxIntTransducerParam
};

static ParamContext_t txrxIntFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_INTFB
};
static const MenuNode_t txrxIntFeedback = {
  .id = MENU_ID_TXRX_INTFB,
  .description = "Integer Through Feedback",
  .handler = transmitIntFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxIntFeedbackParam
};

static ParamContext_t txrxFloatTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_FLOATOUT
};
static const MenuNode_t txrxFloatTransducer = {
  .id = MENU_ID_TXRX_FLOATOUT,
  .description = "Float Through Transducer",
  .handler = transmitFloatOut,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxFloatTransducerParam
};

static ParamContext_t txrxFloatFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_FLOATFB
};
static const MenuNode_t txrxFloatFeedback = {
  .id = MENU_ID_TXRX_FLOATFB,
  .description = "Float Through Feedback",
  .handler = transmitFloatFb,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxFloatFeedbackParam
};

static ParamContext_t txrxTogglePrintParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_TXRX_ENPNT
};
static const MenuNode_t txrxTogglePrint = {
  .id = MENU_ID_TXRX_ENPNT,
  .description = "Enable/Disable Printing Of Received Messages",
  .handler = togglePrint,
  .parent_id = MENU_ID_TXRX,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &txrxTogglePrintParam
};


/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterTxRxMenu()
{
  bool ret = registerMenu(&txrxMenu) && registerMenu(&txrxBitsTransducer) &&
             registerMenu(&txrxStrTransducer) && registerMenu(&txrxIntTransducer) &&
             registerMenu(&txrxFloatTransducer) && registerMenu(&txrxTogglePrint) &&
             registerMenu(&txrxStrFeedback) && registerMenu(&txrxBitsFeedback) &&
             registerMenu(&txrxIntFeedback) && registerMenu(&txrxFloatFeedback);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

void transmitBitsOut(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  transmitBits(context, false);
}
 
void transmitBitsFb(void* argument) {
  FunctionContext_t* context = (FunctionContext_t*) argument;

  transmitBits(context, true);
}

void transmitStringOut(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitString(context, false);
}

void transmitStringFb(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitString(context, true);
}

void transmitIntOut(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitInt(context, false);
}

void transmitIntFb(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitInt(context, true);
}

void transmitFloatOut(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitFloat(context, false);
}

void transmitFloatFb(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  transmitFloat(context, true);
}

void togglePrint(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

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
  if (Param_GetUint8(PARAM_ID, (uint8_t*) &msg->preamble.modem_id.value) == false) {
    COMM_TransmitData("\r\nError getting sender ID. Message not sent\r\n", 
        CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }
  msg->preamble.modem_id.valid = true;
  if (MESS_AddMessageToTxQ(msg) == pdPASS) {
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
