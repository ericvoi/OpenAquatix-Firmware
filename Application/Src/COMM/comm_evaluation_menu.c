/*
 * comm_evaluation_menu.c
 *
 *  Created on: Mar 16, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_function_loops.h"

#include "mess_main.h"
#include "mess_evaluate.h"
#include "mess_packet.h"

#include "cfg_parameters.h"

#include "check_inputs.h"
#include "usb_comm.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void setEvalMsgLen(FunctionContext_t* context);
void sendEvalFeedback(FunctionContext_t* context);
void sendEvalTransducer(FunctionContext_t* context);
void startFeedbackTests(FunctionContext_t* context);

void sendEvalMessage(FunctionContext_t* context, Message_t* msg);

/* Private variables ---------------------------------------------------------*/

extern osMessageQueueId_t regular_tx_queue;

static MenuID_t eval_menu_children[] = {
  MENU_ID_EVAL_SETLEN,      MENU_ID_EVAL_FEEDBACK, 
  MENU_ID_EVAL_TRANSDUCER,  MENU_ID_EVAL_FEEDBACKTESTS
};

static const MenuNode_t eval_menu = {
  .id = MENU_ID_EVAL,
  .description = "Evaluation Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = eval_menu_children,
  .num_children = sizeof(eval_menu_children) / sizeof(eval_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t eval_set_msg_len_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_SETLEN
};
static const MenuNode_t eval_set_msg_len = {
  .id = MENU_ID_EVAL_SETLEN,
  .description = "Set evaluation message length",
  .handler = setEvalMsgLen,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &eval_set_msg_len_param
};

static ParamContext_t eval_feedback_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_FEEDBACK
};
static const MenuNode_t eval_feedback = {
  .id = MENU_ID_EVAL_FEEDBACK,
  .description = "Send evaluation message through feedback network",
  .handler = sendEvalFeedback,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &eval_feedback_param
};

static ParamContext_t eval_transducer_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_TRANSDUCER
};
static const MenuNode_t eval_transducer = {
  .id = MENU_ID_EVAL_TRANSDUCER,
  .description = "Send evaluation message through transducer",
  .handler = sendEvalTransducer,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &eval_transducer_param
};

static ParamContext_t feedback_tests_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_FEEDBACKTESTS
};
static const MenuNode_t feedback_tests = {
  .id = MENU_ID_EVAL_FEEDBACKTESTS,
  .description = "Perform feedback network tests",
  .handler = startFeedbackTests,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &feedback_tests_param
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterEvalMenu(void)
{
  bool ret = MenuSystem_RegisterMenu(&eval_menu) && 
             MenuSystem_RegisterMenu(&eval_set_msg_len) && MenuSystem_RegisterMenu(&eval_feedback) &&
             MenuSystem_RegisterMenu(&eval_transducer) && MenuSystem_RegisterMenu(&feedback_tests);
  return ret;
}


/* Private function definitions ----------------------------------------------*/

void setEvalMsgLen(FunctionContext_t* context)
{
  COMMLoops_LoopUint16(context, PARAM_EVAL_MESSAGE_LEN);
}

void sendEvalFeedback(FunctionContext_t* context)
{
  Message_t msg;
  msg.type = MSG_TRANSMIT_FEEDBACK;
  sendEvalMessage(context, &msg);
}

void sendEvalTransducer(FunctionContext_t* context)
{
  Message_t msg;
  msg.type = MSG_TRANSMIT_TRANSDUCER;
  sendEvalMessage(context, &msg);
}

void startFeedbackTests(FunctionContext_t* context) 
{
  osEventFlagsSet(print_event_handle, MESS_FEEDBACK_TESTS);

  context->state->state = PARAM_STATE_COMPLETE;
}

void sendEvalMessage(FunctionContext_t* context, Message_t* msg)
{
  msg->timestamp = osKernelGetTickCount();
  msg->data_type = EVAL;
  msg->delay = false;
  msg->preamble.message_type.value = EVAL;
  msg->preamble.message_type.valid = true;
  if (Param_GetUint8(PARAM_ID, (uint8_t*) &msg->preamble.modem_id.value) == false) {
    COMM_TransmitData("\r\nError getting sender ID. Message not sent\r\n", 
        CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }
  msg->preamble.modem_id.valid = true;
  if (Param_GetUint16(PARAM_EVAL_MESSAGE_LEN, &msg->length_bits) == false) {
    COMM_TransmitData("\r\nError getting evaluation message length. Message not sent\r\n", 
      CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }
  msg->length_bits *= 8;

  if (osMessageQueuePut(regular_tx_queue, msg, 0, 0) == osOK) {
    sprintf((char*) context->output_buffer, "\r\nSuccessfully added to feedback queue!\r\n\r\n");
  }
  else {
    sprintf((char*) context->output_buffer, "\r\nError adding message to feedback queue\r\n\r\n");
  }
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}
