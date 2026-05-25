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
#include "error_manager.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void setMessagingProtocol(FunctionContext_t* context);
void send_011_01_Transducer(FunctionContext_t* context);
void send_011_01_Feedback(FunctionContext_t* context);
void toggleTxRxFlag(FunctionContext_t* context);
void toggleForwardCapability(FunctionContext_t* context);
void setMessageCoding(FunctionContext_t* context);
void setMessageEncryption(FunctionContext_t* context);
void setJanusDestinationId(FunctionContext_t* context);
void setJanusId(FunctionContext_t* context);

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
  .description = "JANUS Configuration Menu",
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
  .description = "Set data coding",
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
  .description = "Set data encryption",
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
  .description = "Set destination JANUS ID",
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
  .description = "Set modem JANUS ID",
  .handler = setJanusId,
  .parent_id = MENU_ID_JANUS_PARAM,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &janus_sender_param
};

/* Exported function definitions ---------------------------------------------*/

void COMM_RegisterJanusMenu()
{
  bool ret = MenuSystem_RegisterMenu(&janus_menu) && MenuSystem_RegisterMenu(&messaging_protocol) &&
             MenuSystem_RegisterMenu(&janus_param_menu) && MenuSystem_RegisterMenu(&janus_send_menu) &&
             MenuSystem_RegisterMenu(&janus_011_01_feedback) && MenuSystem_RegisterMenu(&janus_011_01_transducer) &&
             MenuSystem_RegisterMenu(&janus_tx_rx) && MenuSystem_RegisterMenu(&janus_forward) &&
             MenuSystem_RegisterMenu(&janus_coding) && MenuSystem_RegisterMenu (&janus_encryption) &&
             MenuSystem_RegisterMenu(&janus_dest) && MenuSystem_RegisterMenu(&janus_sender);
  
  if (ret == false) REGISTER_ERROR(ERROR_MENU_REGISTRATION);
}

/* Private function definitions ----------------------------------------------*/

void setMessagingProtocol(FunctionContext_t* context)
{
  COMMLoops_LoopEnum(context, PARAM_PROTOCOL);
}

void send_011_01_Transducer(FunctionContext_t* context)
{
  transmit_011_01(context, false);
}

void send_011_01_Feedback(FunctionContext_t* context)
{
  transmit_011_01(context, true);
}

void toggleTxRxFlag(FunctionContext_t* context)
{
  COMMLoops_LoopToggle(context, PARAM_TX_RX_ABILITY);
}

void toggleForwardCapability(FunctionContext_t* context)
{
  COMMLoops_LoopToggle(context, PARAM_FORWARD_CAPABILITY);
}

void setMessageCoding(FunctionContext_t* context)
{
  COMMLoops_LoopEnum(context, PARAM_CODING);
}

void setMessageEncryption(FunctionContext_t* context)
{
  COMMLoops_LoopEnum(context, PARAM_ENCRYPTION);
}

void setJanusDestinationId(FunctionContext_t* context)
{
  COMMLoops_LoopUint8(context, PARAM_JANUS_DESTINATION);
}

void setJanusId(FunctionContext_t* context)
{
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

  msg->delay = false;

  memset(&msg->preamble, 0, sizeof(PreambleContent_t));
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
