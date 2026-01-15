/*
 * comm_hist_menu.c
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
#include "sys_temperature.h"
#include <stdio.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void printReceivedMessages(void* argument);
void printSentMessages(void* argument);
void printErrorLog(void* argument);
void printPeakPwr(void* argument);
void printPwrSinceBoot(void* argument);
void printAvgPwr(void* argument);
void printCurrPwr(void* argument);
void printCurrTemp(void* argument);
void printPeakTemp(void* argument);
void printAvgTemp(void* argument);

/* Private variables ---------------------------------------------------------*/

static MenuID_t hist_menu_children[] = {
  MENU_ID_HIST_PWR,   MENU_ID_HIST_RECV,  MENU_ID_HIST_SENT,
  MENU_ID_HIST_ERR,   MENU_ID_HIST_TEMP
};
static const MenuNode_t hist_menu = {
  .id = MENU_ID_HIST,
  .description = "History Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = hist_menu_children,
  .num_children = sizeof(hist_menu_children) / sizeof(hist_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t pwr_hist_menu_children[] = {
  MENU_ID_HIST_PWR_PEAK,  MENU_ID_HIST_PWR_BOOT,
  MENU_ID_HIST_PWR_AVG,   MENU_ID_HIST_PWR_CURR
};
static const MenuNode_t pwr_hist_menu = {
  .id = MENU_ID_HIST_PWR,
  .description = "Power Menu",
  .handler = NULL,
  .parent_id = MENU_ID_HIST,
  .children_ids = pwr_hist_menu_children,
  .num_children = sizeof(pwr_hist_menu_children) / sizeof(pwr_hist_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t temp_hist_menu_children[] = {
  MENU_ID_HIST_TEMP_CURR, MENU_ID_HIST_TEMP_PEAK, MENU_ID_HIST_TEMP_AVG
};
static const MenuNode_t temp_hist_menu = {
  .id = MENU_ID_HIST_TEMP,
  .description = "Temperature Menu",
  .handler = NULL,
  .parent_id = MENU_ID_HIST,
  .children_ids = temp_hist_menu_children,
  .num_children = sizeof(temp_hist_menu_children) / sizeof(temp_hist_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t received_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_RECV
};
static const MenuNode_t received_hist = {
  .id = MENU_ID_HIST_RECV,
  .description = "Print Last 5 Received Messages",
  .handler = printReceivedMessages,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &received_hist_param
};

static ParamContext_t sent_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_SENT,
};
static const MenuNode_t sent_hist = {
  .id = MENU_ID_HIST_SENT,
  .description = "Print Last 5 Sent Messages",
  .handler = printSentMessages,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &sent_hist_param
};

static ParamContext_t err_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_ERR
};
static const MenuNode_t err_hist = {
  .id = MENU_ID_HIST_ERR,
  .description = "Print Error Log",
  .handler = printErrorLog,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &err_hist_param
};

static ParamContext_t peak_pwr_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_PEAK
};
static const MenuNode_t peak_pwr_hist = {
  .id = MENU_ID_HIST_PWR_PEAK,
  .description = "Peak Power",
  .handler = printPeakPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &peak_pwr_hist_param
};

static ParamContext_t boot_pwr_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_BOOT
};
static const MenuNode_t boot_pwr_hist = {
  .id = MENU_ID_HIST_PWR_BOOT,
  .description = "Power Consumption Since Startup",
  .handler = printPwrSinceBoot,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &boot_pwr_hist_param
};

static ParamContext_t avg_pwr_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_AVG
};
static const MenuNode_t avg_pwr_hist = {
  .id = MENU_ID_HIST_PWR_AVG,
  .description = "Average Power",
  .handler = printAvgPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &avg_pwr_hist_param
};

static ParamContext_t curr_pwr_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_CURR
};
static const MenuNode_t curr_pwr_hist = {
  .id = MENU_ID_HIST_PWR_CURR,
  .description = "Current Power",
  .handler = printCurrPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &curr_pwr_hist_param
};

static ParamContext_t curr_temp_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_CURR
};
static const MenuNode_t curr_temp_hist = {
  .id = MENU_ID_HIST_TEMP_CURR,
  .description = "Current Temperature",
  .handler = printCurrTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &curr_temp_hist_param
};

static ParamContext_t peak_temp_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_PEAK
};
static const MenuNode_t peak_temp_hist = {
  .id = MENU_ID_HIST_TEMP_PEAK,
  .description = "Peak Temperature",
  .handler = printPeakTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &peak_temp_hist_param
};

static ParamContext_t avg_temp_hist_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_AVG
};
static const MenuNode_t avg_temp_hist = {
  .id = MENU_ID_HIST_TEMP_AVG,
  .description = "Average Temperature",
  .handler = printAvgTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &avg_temp_hist_param
};



/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterHistoryMenu()
{
  bool ret = registerMenu(&hist_menu) && registerMenu(&pwr_hist_menu) &&
             registerMenu(&temp_hist_menu) && registerMenu(&received_hist) &&
             registerMenu(&sent_hist) && registerMenu(&err_hist) &&
             registerMenu(&peak_pwr_hist) && registerMenu(&boot_pwr_hist) &&
             registerMenu(&avg_pwr_hist) && registerMenu(&curr_pwr_hist) &&
             registerMenu(&curr_temp_hist) && registerMenu(&peak_temp_hist) &&
             registerMenu(&avg_temp_hist);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

// TODO: implement
void printReceivedMessages(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printSentMessages(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printErrorLog(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printPeakPwr(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printPwrSinceBoot(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printAvgPwr(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printCurrPwr(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void printCurrTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  float temp = Temperature_GetCurrent();

  sprintf((char*) context->output_buffer, "\r\nCurrent temperature: %.2f C\r\n",
          temp);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

void printPeakTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  float temp = Temperature_GetPeak();

  sprintf((char*) context->output_buffer, "\r\nPeak temperature: %.2f C\r\n",
          temp);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

void printAvgTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  float temp = Temperature_GetAverage();

  sprintf((char*) context->output_buffer, "\r\nAverage temperature: %.2f C\r\n",
          temp);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}
