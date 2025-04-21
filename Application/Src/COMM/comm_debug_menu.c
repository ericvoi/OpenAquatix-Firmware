/*
 * comm_debug_menu.c
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "comm_menu_registration.h"
#include "comm_menu_system.h"
#include "comm_function_loops.h"
#include "comm_main.h"

#include "cfg_parameters.h"

#include "sys_temperature.h"

#include "WS2812b-driver.h"
#include "PGA113-driver.h"

#include "check_inputs.h"

#include "mess_main.h"
#include "mess_modulate.h"
#include "mess_packet.h"

#include "cmsis_os.h"
#include "main.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

void getGpioStatus(void* argument);
void setLedColourHandler(void* argument);
void printWaveformHandler(void* argument);
void performNoiseAnalysis(void* argument);
void printCurrentTemp(void* argument);
void printCurrentErrors(void* argument);
void printCurrentPowerConsumption(void* argument);
void enterDfuMode(void* argument);
void resetSavedValues(void* argument);

/* Private variables ---------------------------------------------------------*/

static MenuID_t debugMenuChildren[] = {MENU_ID_DBG_GPIO, MENU_ID_DBG_SETLED,
                                       MENU_ID_DBG_PRINT, MENU_ID_DBG_NOISE,
                                       MENU_ID_DBG_TEMP, MENU_ID_DBG_ERR,
                                       MENU_ID_DBG_PWR, MENU_ID_DBG_DFU,
                                       MENU_ID_DBG_RESETCONFIG};
static const MenuNode_t debugMenu = {
  .id = MENU_ID_DBG,
  .description = "Debug Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = debugMenuChildren,
  .num_children = sizeof(debugMenuChildren) / sizeof(debugMenuChildren[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t debugMenuGPIOParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_GPIO
};
static const MenuNode_t debugMenuGPIO = {
  .id = MENU_ID_DBG_GPIO,
  .description = "Get current state of all GPIO inputs and outputs",
  .handler = getGpioStatus,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuGPIOParam
};

static ParamContext_t debugMenuSetLedParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_SETLED
};
static const MenuNode_t debugMenuSetLed = {
  .id = MENU_ID_DBG_SETLED,
  .description = "Set colour of the LED",
  .handler = setLedColourHandler,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuSetLedParam
};

static ParamContext_t debugMenuPrintParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_PRINT
};
static const MenuNode_t debugMenuPrint = {
  .id = MENU_ID_DBG_PRINT,
  .description = "Print out next received waveform",
  .handler = printWaveformHandler,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuPrintParam
};

static ParamContext_t debugMenuNoiseParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_NOISE
};
static const MenuNode_t debugMenuNoise = {
  .id = MENU_ID_DBG_NOISE,
  .description = "Perform noise analysis on input",
  .handler = performNoiseAnalysis,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuNoiseParam
};

static ParamContext_t debugMenuTempParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_TEMP
};
static const MenuNode_t debugMenuTemp = {
  .id = MENU_ID_DBG_TEMP,
  .description = "Get current junction temperature",
  .handler = printCurrentTemp,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuTempParam
};

static ParamContext_t debugMenuErrParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_ERR
};
static const MenuNode_t debugMenuErr = {
  .id = MENU_ID_DBG_ERR,
  .description = "Get current errors",
  .handler = printCurrentErrors,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuErrParam
};

static ParamContext_t debugMenuPwrParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_PWR
};
static const MenuNode_t debugMenuPwr = {
  .id = MENU_ID_DBG_PWR,
  .description = "Get current power consumption",
  .handler = printCurrentPowerConsumption,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuPwrParam
};

static ParamContext_t debugMenuDfuParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_DFU
};
static const MenuNode_t debugMenuDfu = {
  .id = MENU_ID_DBG_DFU,
  .description = "Enter DFU mode to flash new firmware over USB",
  .handler = enterDfuMode,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuDfuParam
};

static ParamContext_t debugMenuResetParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_RESETCONFIG
};
static const MenuNode_t debugMenuReset = {
  .id = MENU_ID_DBG_RESETCONFIG,
  .description = "Reset saved configuration",
  .handler = resetSavedValues,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuResetParam
};


/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterDebugMenu(void)
{
  bool ret = registerMenu(&debugMenu) && registerMenu(&debugMenuGPIO) &&
             registerMenu(&debugMenuSetLed) && registerMenu(&debugMenuPrint) &&
             registerMenu(&debugMenuNoise) && registerMenu(&debugMenuTemp) &&
             registerMenu(&debugMenuErr) && registerMenu(&debugMenuPwr) &&
             registerMenu(&debugMenuDfu) && registerMenu(&debugMenuReset);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

// TODO: implement
void getGpioStatus(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void setLedColourHandler(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  static uint8_t red = 0;
  static uint8_t green = 0;
  static uint8_t blue = 0;

  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0: // Prompt for red
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a red value from 0-255\r\nRed: ");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1: // check red input
        if (checkUint8(context->input, context->input_len, &red, 0, 255) == false) {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input: Value must be a valid integer between 0-255");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
          break;
        } else {
          context->state->state = PARAM_STATE_2;
        }
        // fall through
      case PARAM_STATE_2: // Prompt for green
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a green value from 0-255\r\nGreen: ");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_3;
        break;
      case PARAM_STATE_3: // check green
        if (! checkUint8(context->input, context->input_len, &green, 0, 255)) {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input: Value must be a valid integer between 0-255");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_2;
          break;
        } else {
          context->state->state = PARAM_STATE_4;
        }
        // fall through
      case PARAM_STATE_4: // prompt blue
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a blue value from 0-255\r\nBlue: ");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_5;
        break;
      case PARAM_STATE_5: // check blue
        if (! checkUint8(context->input, context->input_len, &blue, 0, 255)) {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input: Value must be a valid integer between 0-255");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_4;
        } else {
          // Received 3 valid rgb values so now set LED colour

          WS_SetColour(0, red, green, blue);
          WS_Update();

          context->state->state = PARAM_STATE_COMPLETE;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state); // Continues looping if the state has regressed
}

void printWaveformHandler(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (print_event_handle == NULL) {
    return;
  }

  osEventFlagsSet(print_event_handle, MESS_PRINT_WAVEFORM);

  COMM_TransmitData("\r\nThe next waveform will be printed. This function should "
                    "only be used with a script.", CALC_LEN, COMM_USB);

  context->state->state = PARAM_STATE_COMPLETE;
}

void performNoiseAnalysis(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (print_event_handle == NULL) {
    return;
  }

  osEventFlagsSet(print_event_handle, MESS_PRINT_REQUEST);
  uint32_t flags;

  do {
    // Prevents accidentally corrupting noise analysis data
    flags = osEventFlagsWait(print_event_handle, MESS_PRINT_COMPLETE, osFlagsNoClear, osWaitForever);
    osDelay(1);
  } while ((flags & MESS_PRINT_COMPLETE) != MESS_PRINT_COMPLETE);

  osEventFlagsClear(print_event_handle, MESS_PRINT_COMPLETE);

  context->state->state = PARAM_STATE_COMPLETE;
}

void printCurrentTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  float temp = Temperature_GetCurrent();

  sprintf((char*) context->output_buffer, "\r\nCurrent temperature: %.2f C\r\n",
          temp);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

// TODO: implement
void printCurrentErrors(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

// TODO: implement
void printCurrentPowerConsumption(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  
  COMMLoops_NotImplemented(context);
}

void resetSavedValues(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        COMM_TransmitData("\r\nThis will reset the device. Are you sure? (y/n)\r\n", CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        bool affirm;
        if (checkYesNo(*context->input, &affirm) == false) {
          COMM_TransmitData("\r\nInvalid input!\r\n", CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
          break;
        }
        if (affirm == true) {
          COMM_TransmitData("\r\nResetting flash sector...", CALC_LEN, context->comm_interface);
          if (Param_FlashReset() == false) {
            COMM_TransmitData("\r\nError encountered. Aborting...", CALC_LEN, context->comm_interface);
            context->state->state = PARAM_STATE_COMPLETE;
            break;
          }
          COMM_TransmitData("\r\nResetting device...\r\n", CALC_LEN, context->comm_interface);

          HAL_NVIC_SystemReset();
        }
        context->state->state = PARAM_STATE_COMPLETE;
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void enterDfuMode(void* argument)
{
  (void)(argument);

  // write magic number to magic address. See startup code for corresponding check
  *((uint32_t*) 0x38000000) = 0xABCDABCD;

  osDelay(10);

  NVIC_SystemReset();
}
