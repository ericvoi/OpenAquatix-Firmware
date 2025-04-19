/*
 * comm_hist_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_function_loops.h"
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

static MenuID_t histMenuChildren[] = {
  MENU_ID_HIST_PWR,   MENU_ID_HIST_RECV,  MENU_ID_HIST_SENT, 
  MENU_ID_HIST_ERR,   MENU_ID_HIST_ERR,   MENU_ID_HIST_TEMP
};
static const MenuNode_t histMenu = {
  .id = MENU_ID_HIST,
  .description = "History Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = histMenuChildren,
  .num_children = sizeof(histMenuChildren) / sizeof(histMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t pwrHistMenuChildren[] = {
  MENU_ID_HIST_PWR_PEAK,  MENU_ID_HIST_PWR_BOOT,
  MENU_ID_HIST_PWR_AVG,   MENU_ID_HIST_PWR_CURR
};
static const MenuNode_t pwrHistMenu = {
  .id = MENU_ID_HIST_PWR,
  .description = "Power Menu",
  .handler = NULL,
  .parent_id = MENU_ID_HIST,
  .children_ids = pwrHistMenuChildren,
  .num_children = sizeof(pwrHistMenuChildren) / sizeof(pwrHistMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static MenuID_t tempHistMenuChildren[] = {
  MENU_ID_HIST_TEMP_CURR, MENU_ID_HIST_TEMP_PEAK, MENU_ID_HIST_TEMP_AVG
};
static const MenuNode_t tempHistMenu = {
  .id = MENU_ID_HIST_TEMP,
  .description = "Temperature Menu",
  .handler = NULL,
  .parent_id = MENU_ID_HIST,
  .children_ids = tempHistMenuChildren,
  .num_children = sizeof(tempHistMenuChildren) / sizeof(tempHistMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t receivedHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_RECV
};
static const MenuNode_t receivedHist = {
  .id = MENU_ID_HIST_RECV,
  .description = "Print Last 5 Received Messages",
  .handler = printReceivedMessages,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &receivedHistParam
};

static ParamContext_t sentHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_SENT,
};
static const MenuNode_t sentHist = {
  .id = MENU_ID_HIST_SENT,
  .description = "Print Last 5 Sent Messages",
  .handler = printSentMessages,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &sentHistParam
};

static ParamContext_t errHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_ERR
};
static const MenuNode_t errHist = {
  .id = MENU_ID_HIST_ERR,
  .description = "Print Error Log",
  .handler = printErrorLog,
  .parent_id = MENU_ID_HIST,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &errHistParam
};

static ParamContext_t peakPwrHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_PEAK
};
static const MenuNode_t peakPwrHist = {
  .id = MENU_ID_HIST_PWR_PEAK,
  .description = "Peak Power",
  .handler = printPeakPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &peakPwrHistParam
};

static ParamContext_t bootPwrHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_BOOT
};
static const MenuNode_t bootPwrHist = {
  .id = MENU_ID_HIST_PWR_BOOT,
  .description = "Power Consumption Since Startup",
  .handler = printPwrSinceBoot,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &bootPwrHistParam
};

static ParamContext_t avgPwrHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_AVG
};
static const MenuNode_t avgPwrHist = {
  .id = MENU_ID_HIST_PWR_AVG,
  .description = "Average Power",
  .handler = printAvgPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &avgPwrHistParam
};

static ParamContext_t currPwrHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_PWR_CURR
};
static const MenuNode_t currPwrHist = {
  .id = MENU_ID_HIST_PWR_CURR,
  .description = "Current Power",
  .handler = printCurrPwr,
  .parent_id = MENU_ID_HIST_PWR,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &currPwrHistParam
};

static ParamContext_t currTempHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_CURR
};
static const MenuNode_t currTempHist = {
  .id = MENU_ID_HIST_TEMP_CURR,
  .description = "Current Temperature",
  .handler = printCurrTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &currTempHistParam
};

static ParamContext_t peakTempHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_PEAK
};
static const MenuNode_t peakTempHist = {
  .id = MENU_ID_HIST_TEMP_PEAK,
  .description = "Peak Temperature",
  .handler = printPeakTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &peakTempHistParam
};

static ParamContext_t avgTempHistParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_HIST_TEMP_AVG
};
static const MenuNode_t avgTempHist = {
  .id = MENU_ID_HIST_TEMP_AVG,
  .description = "Average Temperature",
  .handler = printAvgTemp,
  .parent_id = MENU_ID_HIST_TEMP,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &avgTempHistParam
};



/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterHistoryMenu()
{
  bool ret = registerMenu(&histMenu) && registerMenu(&pwrHistMenu) &&
             registerMenu(&tempHistMenu) && registerMenu(&receivedHist) &&
             registerMenu(&sentHist) && registerMenu(&errHist) &&
             registerMenu(&peakPwrHist) && registerMenu(&bootPwrHist) &&
             registerMenu(&avgPwrHist) && registerMenu(&currPwrHist) &&
             registerMenu(&currTempHist) && registerMenu(&peakTempHist) &&
             registerMenu(&avgTempHist);
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

// TODO: implement
void printCurrTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printPeakTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printAvgTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}
