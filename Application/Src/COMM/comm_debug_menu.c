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

#include "WS2812b-driver.h"
#include "PGA113-driver.h"

#include "comm_main.h"

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
void sendFeedbackSignal(void* argument);
void sendTransducerSignal(void* argument);
void changeOutputAmplitude(void* argument);
void changePgaGain(void* argument);
void sendTestTransducerSignal(void* argument);

/* Private variables ---------------------------------------------------------*/

static MenuID_t debugMenuChildren[] = {MENU_ID_DBG_GPIO, MENU_ID_DBG_SETLED,
                                       MENU_ID_DBG_PRINT, MENU_ID_DBG_NOISE,
                                       MENU_ID_DBG_TEMP, MENU_ID_DBG_ERR,
                                       MENU_ID_DBG_PWR, MENU_ID_DBG_SEND,
                                       MENU_ID_DBG_SENDOUT, MENU_ID_DBG_OUTAMP,
                                       MENU_ID_DBG_INGAIN, MENU_ID_DBG_TESTOUT};
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

static ParamContext_t debugMenuSendParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_SEND
};
static const MenuNode_t debugMenuSend = {
  .id = MENU_ID_DBG_SEND,
  .description = "[TEMP] Send Test Waveform Through Feedback Network",
  .handler = sendFeedbackSignal,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuSendParam
};

static ParamContext_t debugMenuSendTransducerParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_SENDOUT
};
static const MenuNode_t debugMenuSendTransducer = {
  .id = MENU_ID_DBG_SENDOUT,
  .description = "[TEMP] Send Test Waveform Through Transducer",
  .handler = sendTransducerSignal,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuSendTransducerParam
};

static ParamContext_t debugMenuOutAmpParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_OUTAMP
};
static const MenuNode_t debugMenuOutAmp = {
  .id = MENU_ID_DBG_OUTAMP,
  .description = "[TEMP] Change fixed output amplitude",
  .handler = changeOutputAmplitude,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuOutAmpParam
};

static ParamContext_t debugMenuPgaGainParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_INGAIN
};
static const MenuNode_t debugMenuPgaGain = {
  .id = MENU_ID_DBG_INGAIN,
  .description = "[TEMP] Manually change the PGAs gain",
  .handler = changePgaGain,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuPgaGainParam
};

static ParamContext_t debugMenuSendOutParam = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_TESTOUT
};
static const MenuNode_t debugMenuSendOut = {
  .id = MENU_ID_DBG_TESTOUT,
  .description = "[TEMP] Send a test waveform through transducer",
  .handler = sendTestTransducerSignal,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debugMenuSendOutParam
};


/* Exported function definitions ---------------------------------------------*/

bool COMM_RegisterDebugMenu(void)
{
  bool ret = registerMenu(&debugMenu) && registerMenu(&debugMenuGPIO) &&
             registerMenu(&debugMenuSetLed) && registerMenu(&debugMenuPrint) &&
             registerMenu(&debugMenuNoise) && registerMenu(&debugMenuTemp) &&
             registerMenu(&debugMenuErr) && registerMenu(&debugMenuPwr) &&
             registerMenu(&debugMenuSend) && registerMenu(&debugMenuSendTransducer) &&
             registerMenu(&debugMenuOutAmp) && registerMenu(&debugMenuPgaGain) &&
             registerMenu(&debugMenuSendOut);
  return ret;
}

/* Private function definitions ----------------------------------------------*/

void getGpioStatus(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  context->state->state = PARAM_STATE_COMPLETE;
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
  context->state->state = PARAM_STATE_COMPLETE;
}

void performNoiseAnalysis(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (print_event_handle == NULL) return;

  osEventFlagsSet(print_event_handle, MESS_PRINT_REQUEST);
  uint32_t flags;

  do {
    // Prevents accidentally corrupting noise analysis data
    flags = osEventFlagsWait(print_event_handle, MESS_PRINT_COMPLETE, osFlagsWaitAny, osWaitForever);
    osDelay(1);
  } while ((flags & MESS_PRINT_COMPLETE) != MESS_PRINT_COMPLETE);

  osEventFlagsClear(print_event_handle, MESS_PRINT_COMPLETE);

  context->state->state = PARAM_STATE_COMPLETE;
}

void printCurrentTemp(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  context->state->state = PARAM_STATE_COMPLETE;
}

void printCurrentErrors(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  context->state->state = PARAM_STATE_COMPLETE;
}

void printCurrentPowerConsumption(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;
  context->state->state = PARAM_STATE_COMPLETE;
}

void sendFeedbackSignal(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a string to send to the feedback network with a maximum length of 8 characters:\r\n");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        if (context->input_len > 8) {
          sprintf((char*) context->output_buffer, "\r\nInput string must be less than 8 characters!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = MSG_TRANSMIT_FEEDBACK;
          msg.length_bits = TEST_PACKET_LENGTH;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = STRING;
          for (uint16_t i = 0; i < TEST_PACKET_LENGTH / 8; i++) {
            if (context->input_len > i) {
              msg.data[i] = context->input[i];
            }
            else {
              msg.data[i] = ' ';
            }
          }
          if (MESS_AddMessageToTxQ(&msg) == pdPASS) {
            sprintf((char*) context->output_buffer, "\r\nSuccessfully added to feedback queue!\r\n\r\n");
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          }
          else {
            sprintf((char*) context->output_buffer, "\r\nError adding message to feedback queue\r\n\r\n");
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state); // Continues looping if the state has regressed
}

void sendTransducerSignal(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        sprintf((char*) context->output_buffer, "\r\n\r\nPlease enter a string to send through the transducer with a maximum length of 8 characters:\r\n");
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;

      case PARAM_STATE_1:
        if (context->input_len > 8) {
          sprintf((char*) context->output_buffer, "\r\nInput string must be less than 8 characters!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        else {
          Message_t msg;
          msg.type = MSG_TRANSMIT_TRANSDUCER;
          msg.length_bits = TEST_PACKET_LENGTH;
          msg.timestamp = osKernelGetTickCount();
          msg.data_type = STRING;
          for (uint16_t i = 0; i < TEST_PACKET_LENGTH / 8; i++) {
            if (context->input_len > i) {
              msg.data[i] = context->input[i];
            }
            else {
              msg.data[i] = ' ';
            }
          }
          if (MESS_AddMessageToTxQ(&msg) == pdPASS) {
            sprintf((char*) context->output_buffer, "\r\nSuccessfully added to output queue!\r\n\r\n");
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          }
          else {
            sprintf((char*) context->output_buffer, "\r\nError adding message to output queue\r\n\r\n");
            COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          }
          context->state->state = PARAM_STATE_COMPLETE;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state); // Continues looping if the state has regressed
}

void changeOutputAmplitude(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  COMMLoops_LoopFloat(context, PARAM_OUTPUT_AMPLITUDE);
}

// temporary before automatic gain control
void changePgaGain(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  ParamState_t old_state = context->state->state;

  do {
    switch (context->state->state) {
      case PARAM_STATE_0:
        uint8_t current_gain = PGA_GetGain();
        sprintf((char*) context->output_buffer, "\r\n\r\nEnter a gain code from 0-7:\r\n0: 1\r\n1: 2\r\n2: 5\r\n3: 10\r\n4: 20\r\n5: 50\r\n6: 100\r\n7: 200\r\nCurrent gain code is %d\r\n", current_gain);
        COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
        context->state->state = PARAM_STATE_1;
        break;
      case PARAM_STATE_1:
        uint8_t newGain = 0;
        if (checkUint8(context->input, context->input_len, &newGain, 0, 7) == true) {
          PGA_SetGain(newGain);
          sprintf((char*) context->output_buffer, "\r\nSuccessfully set the PGA gain code to %d\r\n\r\n", newGain);
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_COMPLETE;
        }
        else {
          sprintf((char*) context->output_buffer, "\r\nInvalid Input!\r\n");
          COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
          context->state->state = PARAM_STATE_0;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state);
}

void sendTestTransducerSignal(void* argument)
{
  FunctionContext_t* context = (FunctionContext_t*) argument;

  if (print_event_handle == NULL) return;

  osEventFlagsSet(print_event_handle, MESS_TEST_OUTPUT);

  context->state->state = PARAM_STATE_COMPLETE;
}
