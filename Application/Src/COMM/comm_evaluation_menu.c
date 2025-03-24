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

void toggleEval(void* argument);
void setEvalMsg(void* argument);
void sendEvalFeedback(void* argument);
void sendEvalTransducer(void* argument);

/* Private variables ---------------------------------------------------------*/

static MenuID_t evalMenuChildren[] = {
  MENU_ID_EVAL_TOGGLE,    MENU_ID_EVAL_SETMSG, MENU_ID_EVAL_FEEDBACK, 
  MENU_ID_EVAL_TRANSDUCER
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

static ParamContext_t evalToggleModeParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_TOGGLE
};
static const MenuNode_t evalToggleMode = {
  .id = MENU_ID_EVAL_TOGGLE,
  .description = "Toggle Evaluation Mode",
  .handler = toggleEval,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalToggleModeParam
};

static ParamContext_t evalSetMsgParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_SETMSG
};
static const MenuNode_t evalSetMsg = {
  .id = MENU_ID_EVAL_SETMSG,
  .description = "Set Evaluation Message",
  .handler = setEvalMsg,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalSetMsgParam
};

static ParamContext_t evalFeedbackParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_EVAL_FEEDBACK
};
static const MenuNode_t evalFeedback = {
  .id = MENU_ID_EVAL_FEEDBACK,
  .description = "Send Evaluation Message Through Feedback Network",
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
  .description = "Send Evaluation Message Through Transducer",
  .handler = sendEvalTransducer,
  .parent_id = MENU_ID_EVAL,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &evalTransducerParam
};

/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterEvalMenu(void)
{
  bool ret = registerMenu(&evalMenu) && registerMenu(&evalToggleMode) &&
             registerMenu(&evalSetMsg) && registerMenu(&evalFeedback) &&
             registerMenu(&evalTransducer);
  return ret;
}


/* Private function definitions ----------------------------------------------*/

void toggleEval(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopToggle(context, PARAM_EVAL_MODE_ON);
}
void setEvalMsg(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopUint8(context, PARAM_EVAL_MESSAGE);
}

void sendEvalFeedback(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  Message_t msg;
  msg.type = MSG_TRANSMIT_FEEDBACK;
  msg.timestamp = osKernelGetTickCount();
  msg.data_type = EVAL;
  Evaluate_CopyEvaluationMessage(&msg);
  msg.eval_info = NULL;

  if (MESS_AddMessageToTxQ(&msg) == pdPASS) {
    sprintf((char*) context->output_buffer, "\r\nSuccessfully added to feedback queue!\r\n\r\n");
  }
  else {
    sprintf((char*) context->output_buffer, "\r\nError adding message to feedback queue\r\n\r\n");
  }
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}

void sendEvalTransducer(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  Message_t msg;
  msg.type = MSG_TRANSMIT_TRANSDUCER;
  msg.timestamp = osKernelGetTickCount();
  msg.data_type = EVAL;
  Evaluate_CopyEvaluationMessage(&msg);
  msg.eval_info = NULL;

  if (MESS_AddMessageToTxQ(&msg) == pdPASS) {
    sprintf((char*) context->output_buffer, "\r\nSuccessfully added to ouput queue!\r\n\r\n");
  }
  else {
    sprintf((char*) context->output_buffer, "\r\nError adding message to output queue\r\n\r\n");
  }
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);

  context->state->state = PARAM_STATE_COMPLETE;
}