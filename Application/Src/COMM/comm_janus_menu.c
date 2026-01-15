/*
 * comm_janus_menu.c
 *
 *  Created on: Aug 6, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_function_loops.h"
#include "comm_main.h"
#include "mess_main.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void setMessagingProtocol(void* argument);
void send_011_01_Transducer(void* argument);
void send_011_01_Feedback(void* argument);
void toggleTxRxFlag(void* argument);
void toggleForwardCapability(void* argument);
void setMessageCoding(void* argument);
void setMessageEncryption(void* argument);
void setJanusDestinationId(void* argument);
void setJanusId(void* argument);

static void transmit_011_01(FunctionContext_t* context, bool is_feedback);
static void sendMessageToTxQueue(FunctionContext_t* context, Message_t* msg, bool is_feedback);
static bool inJanusMode(FunctionContext_t* context);

/* Private variables ---------------------------------------------------------*/

extern osMessageQueueId_t regular_tx_queue;

static MenuID_t janus_menu_children[] = {
  MENU_ID_JANUS_PROTOCOL, MENU_ID_JANUS_SEND, MENU_ID_JANUS_PARAM
};
static const MenuNode_t janus_menu = {
  .id = MENU_ID_JANUS,
  .description = "JANUS Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = janus_menu_children,
  .num_children = sizeof(janus_menu_children) / sizeof(janus_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t messaging_protocol_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PROTOCOL
};
static const MenuNode_t messaging_protocol = {
  .id = MENU_ID_JANUS_PROTOCOL,
  .description = "Set messaging protocol",
  .handler = setMessagingProtocol,
  .parent_id = MENU_ID_JANUS,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &messaging_protocol_param
};


static MenuID_t janus_send_menu_children[] = {
  MENU_ID_JANUS_SEND_011_01_OUT, MENU_ID_JANUS_SEND_011_01_FB
};
static const MenuNode_t janus_send_menu = {
  .id = MENU_ID_JANUS_SEND,
  .description = "Send JANUS message",
  .handler = NULL,
  .parent_id = MENU_ID_JANUS,
  .children_ids = janus_send_menu_children,
  .num_children = sizeof(janus_send_menu_children) / sizeof(janus_send_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t janus_param_menu_children[] = {
  MENU_ID_JANUS_PARAM_TXRX, MENU_ID_JANUS_PARAM_FORWARD, 
  MENU_ID_JANUS_PARAM_CODING, MENU_ID_JANUS_PARAM_ENC,
  MENU_ID_JANUS_PARAM_DEST, MENU_ID_JANUS_PARAM_SENDER
};
static const MenuNode_t janus_param_menu = {
  .id = MENU_ID_JANUS_PARAM,
  .description = "Modify JANUS parameters",
  .handler = NULL,
  .parent_id = MENU_ID_JANUS,
  .children_ids = janus_param_menu_children,
  .num_children = sizeof(janus_param_menu_children) / sizeof(janus_param_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t janus_011_01_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_SEND_011_01_OUT
};
static const MenuNode_t janus_011_01_transducer = {
  .id = MENU_ID_JANUS_SEND_011_01_OUT,
  .description = "Send JANUS 011 01 (SMS) through transducer",
  .handler = send_011_01_Transducer,
  .parent_id = MENU_ID_JANUS_SEND,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_011_01_transducer_param
};

static ParamContext_t janus_011_01_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_SEND_011_01_FB
};
static const MenuNode_t janus_011_01_feedback = {
  .id = MENU_ID_JANUS_SEND_011_01_FB,
  .description = "Send JANUS 011 01 (SMS) through feedback network",
  .handler = send_011_01_Feedback,
  .parent_id = MENU_ID_JANUS_SEND,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_011_01_feedback_param
};

static ParamContext_t janus_tx_rx_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_TXRX
};
static const MenuNode_t janus_tx_rx = {
  .id = MENU_ID_JANUS_PARAM_TXRX,
  .description = "Toggle Tx/Rx capability",
  .handler = toggleTxRxFlag,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_tx_rx_param
};

static ParamContext_t janus_forward_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_FORWARD
};
static const MenuNode_t janus_forward = {
  .id = MENU_ID_JANUS_PARAM_FORWARD,
  .description = "Toggle forwarding capability",
  .handler = toggleForwardCapability,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_forward_param
};

static ParamContext_t janus_coding_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_CODING
};
static const MenuNode_t janus_coding = {
  .id = MENU_ID_JANUS_PARAM_CODING,
  .description = "Modify data coding",
  .handler = setMessageCoding,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_coding_param
};

static ParamContext_t janus_encryption_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_ENC
};
static const MenuNode_t janus_encryption = {
  .id = MENU_ID_JANUS_PARAM_ENC,
  .description = "Modify data encryption",
  .handler = setMessageEncryption,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_encryption_param
};

static ParamContext_t janus_dest_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_DEST
};
static const MenuNode_t janus_dest = {
  .id = MENU_ID_JANUS_PARAM_DEST,
  .description = "Destination's JANUS ID",
  .handler = setJanusDestinationId,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_dest_param
};

static ParamContext_t janus_sender_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_JANUS_PARAM_SENDER
};
static const MenuNode_t janus_sender = {
  .id = MENU_ID_JANUS_PARAM_SENDER,
  .description = "Modem's JANUS ID",
  .handler = setJanusId,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_sender_param
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterJanusMenu()
{
  bool ret = registerMenu(&janus_menu) && registerMenu(&messaging_protocol) &&
             registerMenu(&janus_param_menu) && registerMenu(&janus_send_menu) &&
             registerMenu(&janus_011_01_feedback) && registerMenu(&janus_011_01_transducer) &&
             registerMenu(&janus_tx_rx) && registerMenu(&janus_forward) &&
             registerMenu(&janus_coding) && registerMenu (&janus_encryption) &&
             registerMenu(&janus_dest) && registerMenu(&janus_sender);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

void setMessagingProtocol(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  char* descriptors[] = {"Custom", "JANUS"};

  COMMLoops_LoopEnum(context, PARAM_PROTOCOL, descriptors, sizeof(descriptors) / sizeof(descriptors[0]));
}

void send_011_01_Transducer(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  transmit_011_01(context, false);
}

void send_011_01_Feedback(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  transmit_011_01(context, true);
}

void toggleTxRxFlag(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_TX_RX_ABILITY);
}

void toggleForwardCapability(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_FORWARD_CAPABILITY);
}

void setMessageCoding(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"8 bit ASCII", "7 bit ASCII", "6 bit ASCII (AIS)", "UTF-8"};

  COMMLoops_LoopEnum(context, PARAM_CODING, descriptors, sizeof(descriptors) / sizeof(descriptors[0]));
}

void setMessageEncryption(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  char* descriptors[] = {"No encryption",                     "AES-GCM (not yet implemented)", 
                         "user type 2 (not yet implemented)", "user type 3 (not yet implemented)",
                         "user type 4 (not yet implemented)", "user type 5 (not yet implemented)",
                         "user type 6 (not yet implemented)", "user type 7 (not yet implemented)"};

  COMMLoops_LoopEnum(context, PARAM_ENCRYPTION, descriptors, sizeof(descriptors) / sizeof(descriptors[0]));
}

void setJanusDestinationId(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_JANUS_DESTINATION);
}

void setJanusId(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_JANUS_ID);
}

// TODO: let maximum message lengths take into account coding decreasing message size.
// This would require coding to be applied when the message is received
void transmit_011_01(FunctionContext_t* context, bool is_feedback)
{
  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a SMS to "
            "send to the %s with a maximum length of %u characters:\r\n", 
            is_feedback ? "feedback network" : "transducer",
            PACKET_DATA_MAX_LENGTH_BYTES);
        COMM_TransmitData(context->output_buffer, CALC_LEN, 
            context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        if (context->input_len > 128) {
          sprintf((char*) context->output_buffer, "\r\nInput SMS must be"
              "less than %u characters!\r\n", PACKET_DATA_MAX_LENGTH_BYTES);
          COMM_TransmitData(context->output_buffer, CALC_LEN, 
              context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = is_feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER;
          msg.timestamp = osKernelGetTickCount();
          msg.janus_data_type = JANUS_011_01_SMS;
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

void sendMessageToTxQueue(FunctionContext_t* context, Message_t* msg, bool is_feedback)
{
  if (inJanusMode(context) == false) return;

  memset(&msg->preamble, 0, sizeof(PreambleContent_t));
  if (osMessageQueuePut(regular_tx_queue, msg, 0, 0) == true) {
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

bool inJanusMode(FunctionContext_t* context)
{
  MessagingProtocol_t protocol;
  if (Param_GetUint8(PARAM_PROTOCOL, &protocol) == false) {
    context->state->state = PARAM_STATE_COMPLETE;
    COMM_TransmitData("Cannot find protocol information. Message not sent.\r\n", CALC_LEN, context->comm_interface);
    return false;
  }

  if (protocol == PROTOCOL_JANUS) {
    return true;
  }

  context->state->state = PARAM_STATE_COMPLETE;
  COMM_TransmitData("Cannot send JANUS messages in non-JANUS modes. Message not sent\r\n", CALC_LEN, context->comm_interface);
  return false;
}
