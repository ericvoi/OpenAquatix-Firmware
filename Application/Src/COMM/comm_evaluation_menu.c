/*
 * comm_evaluation_menu.c
 *
 *  Created on: Mar 16, 2025
 *      Author: ericv
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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void setEvalMsgLen(void* argument);
void sendEvalFeedback(void* argument);
void sendEvalTransducer(void* argument);
void startFeedbackTests(void* argument);

void sendEvalMessage(FunctionContext_t* context, Message_t* msg);

/* Private variables ---------------------------------------------------------*/

static MenuID_t evalMenuChildren[] = {
  MENU_ID_EVAL_SETLEN,      MENU_ID_EVAL_FEEDBACK, 
  MENU_ID_EVAL_TRANSDUCER,  MENU_ID_EVAL_FEEDBACKTESTS
};

static const MenuNode_t evalMenu = {
  .id = MENU_ID_EVAL,
  .description = "Evaluation Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = evalMenuChildren,
  .num_children = sizeof(evalMenuChildren) / sizeof(evalMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t evalSetMsgLenParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_SETLEN
};
static const MenuNode_t evalSetMsgLen = {
  .id = MENU_ID_EVAL_SETLEN,
  .description = "Set evaluation message length",
  .handler = setEvalMsgLen,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalSetMsgLenParam
};

static ParamContext_t evalFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_FEEDBACK
};
static const MenuNode_t evalFeedback = {
  .id = MENU_ID_EVAL_FEEDBACK,
  .description = "Send evaluation message through feedback network",
  .handler = sendEvalFeedback,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalFeedbackParam
};

static ParamContext_t evalTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_TRANSDUCER
};
static const MenuNode_t evalTransducer = {
  .id = MENU_ID_EVAL_TRANSDUCER,
  .description = "Send evaluation message through transducer",
  .handler = sendEvalTransducer,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalTransducerParam
};

static ParamContext_t feedbackTestsParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_FEEDBACKTESTS
};
static const MenuNode_t feedbackTests = {
  .id = MENU_ID_EVAL_FEEDBACKTESTS,
  .description = "Perform feedback network tests",
  .handler = startFeedbackTests,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &feedbackTestsParam
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterEvalMenu(void)
{
  bool ret = registerMenu(&evalMenu) && 
             registerMenu(&evalSetMsgLen) && registerMenu(&evalFeedback) &&
             registerMenu(&evalTransducer) && registerMenu(&feedbackTests);
  return ret;
}


/* Private function definitions ----------------------------------------------*/

void setEvalMsgLen(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint16(context, PARAM_EVAL_MESSAGE_LEN);
}

void sendEvalFeedback(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  Message_t msg;
  msg.type = MSG_TRANSMIT_FEEDBACK;
  sendEvalMessage(context, &msg);
}

void sendEvalTransducer(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  Message_t msg;
  msg.type = MSG_TRANSMIT_TRANSDUCER;
  sendEvalMessage(context, &msg);
}

void startFeedbackTests(void* argument) 
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  osEventFlagsSet(print_event_handle, MESS_FEEDBACK_TESTS);

  context->state->state = PARAM_STATE_COMPLETE;
}

void sendEvalMessage(FunctionContext_t* context, Message_t* msg)
{
  msg->timestamp = osKernelGetTickCount();
  msg->data_type = EVAL;
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

  if (MESS_AddMessageToTxQ(msg) == pdPASS) {
    sprintf((char*) context->output_buffer, "\r\nSuccessfully added to feedback queue!\r\n\r\n");
  }
  else {
    sprintf((char*) context->output_buffer, "\r\nError adding message to feedback queue\r\n\r\n");
  }
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}