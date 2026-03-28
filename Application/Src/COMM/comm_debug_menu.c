/*
 * comm_debug_menu.c
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
#include "comm_main.h"

#include "cfg_parameters.h"
#include "cfg_import_export.h"

#include "sys_temperature.h"
#include "sys_led.h"
#include "sys_main.h"
#include "sys_power.h"

#include "check_inputs.h"

#include "mess_main.h"
#include "mess_modulate.h"
#include "mess_packet.h"
#include "mess_background_noise.h"

#include "cmsis_os.h"
#include "main.h"
#include "error_manager.h"
#include "error_log.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define MAGIC_RESET_NUMBER            0xABCDABCD

/* Private macro -------------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void getGpioStatus(FunctionContext_t* context);
static void setLedColourHandler(FunctionContext_t* context);
static void printWaveformHandler(FunctionContext_t* context);
static void dumpAdcData(FunctionContext_t* context);
static void noiseSpectralAnalysis(FunctionContext_t* context);
static void printCurrentTemp(FunctionContext_t* context);
static void printErrorLog(FunctionContext_t* context);
static void printCurrentPowerConsumption(FunctionContext_t* context);
static void printBackgroundNoise(FunctionContext_t* context);
static void enterDfuMode(FunctionContext_t* context);
static void resetSavedValues(FunctionContext_t* context);
static void deepSleep(FunctionContext_t* context);
static void exportAllParameters(FunctionContext_t* context);
static void importAllParameters(FunctionContext_t* context);

/* Private variables ---------------------------------------------------------*/

extern osEventFlagsId_t sleep_events;

static MenuID_t debug_menu_children[] = {MENU_ID_DBG_GPIO, MENU_ID_DBG_SETLED,
                                         MENU_ID_DBG_PRINT, MENU_ID_DBG_BGDUMP,
                                         MENU_ID_DBG_BGFREQ, MENU_ID_DBG_TEMP, 
                                         MENU_ID_DBG_ERR, MENU_ID_DBG_PWR, 
                                         MENU_ID_DBG_NOISE, MENU_ID_DBG_DFU, 
                                         MENU_ID_DBG_RESETCONFIG, MENU_ID_DBG_DEEPSLEEP,
                                         MENU_ID_DBG_EXPALL, MENU_ID_DBG_IMPALL};
static const MenuNode_t debug_menu = {
  .id = MENU_ID_DBG,
  .description = "Debug Menu",
  .handler = NULL,
  .parent_id = MENU_ID_MAIN,
  .children_ids = debug_menu_children,
  .num_children = sizeof(debug_menu_children) / sizeof(debug_menu_children[0]),
  .access_level = 0,
  .parameters = NULL
};

static ParamContext_t debug_menu_gpio_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_GPIO
};
static const MenuNode_t debug_menu_gpio = {
  .id = MENU_ID_DBG_GPIO,
  .description = "Get current state of all GPIO inputs and outputs",
  .handler = getGpioStatus,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_gpio_param
};

static ParamContext_t debug_menu_set_led_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_SETLED
};
static const MenuNode_t debug_menu_set_led = {
  .id = MENU_ID_DBG_SETLED,
  .description = "Set colour of the LED",
  .handler = setLedColourHandler,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_set_led_param
};

static ParamContext_t debug_menu_print_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_PRINT
};
static const MenuNode_t debug_menu_print = {
  .id = MENU_ID_DBG_PRINT,
  .description = "Print out next received waveform",
  .handler = printWaveformHandler,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_print_param
};

static ParamContext_t debug_menu_noise_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_BGDUMP
};
static const MenuNode_t debug_menu_noise = {
  .id = MENU_ID_DBG_BGDUMP,
  .description = "Get 1000 ADC samples",
  .handler = dumpAdcData,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_noise_param
};

static ParamContext_t debug_menu_noise_f_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_BGFREQ
};
static const MenuNode_t debug_menu_noise_f = {
  .id = MENU_ID_DBG_BGFREQ,
  .description = "Get frequency content of background noise (FFT)",
  .handler = noiseSpectralAnalysis,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_noise_f_param
};

static ParamContext_t debug_menu_temp_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_TEMP
};
static const MenuNode_t debug_menu_temp = {
  .id = MENU_ID_DBG_TEMP,
  .description = "Get current junction temperature",
  .handler = printCurrentTemp,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_temp_param
};

static ParamContext_t debug_menu_err_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_ERR
};
static const MenuNode_t debug_menu_err = {
  .id = MENU_ID_DBG_ERR,
  .description = "Print error log",
  .handler = printErrorLog,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_err_param
};

static ParamContext_t debug_menu_pwr_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_PWR
};
static const MenuNode_t debug_menu_pwr = {
  .id = MENU_ID_DBG_PWR,
  .description = "Get current power consumption",
  .handler = printCurrentPowerConsumption,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_pwr_param
};

static ParamContext_t debug_menu_noise_level_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_NOISE
};
static const MenuNode_t debug_menu_noise_level = {
  .id = MENU_ID_DBG_NOISE,
  .description = "Get background Noise Spectral Density",
  .handler = printBackgroundNoise,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_noise_level_param
};

static ParamContext_t debug_menu_dfu_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_DFU
};
static const MenuNode_t debug_menu_dfu = {
  .id = MENU_ID_DBG_DFU,
  .description = "Enter DFU mode to flash new firmware over USB",
  .handler = enterDfuMode,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_dfu_param
};

static ParamContext_t debug_menu_reset_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_RESETCONFIG
};
static const MenuNode_t debug_menu_reset = {
  .id = MENU_ID_DBG_RESETCONFIG,
  .description = "Reset saved configuration",
  .handler = resetSavedValues,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_reset_param
};

static ParamContext_t debug_menu_deep_sleep_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_DEEPSLEEP
};
static const MenuNode_t debug_menu_deep_sleep = {
  .id = MENU_ID_DBG_DEEPSLEEP,
  .description = "Enter deep sleep mode",
  .handler = deepSleep,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_deep_sleep_param
};

static ParamContext_t debug_menu_expall_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_EXPALL
};
static const MenuNode_t debug_menu_expall = {
  .id = MENU_ID_DBG_EXPALL,
  .description = "Export all parameters",
  .handler = exportAllParameters,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_expall_param
};

static ParamContext_t debug_menu_impall_param = {
  .state = PARAM_STATE_0,
  .param_id = MENU_ID_DBG_IMPALL
};
static const MenuNode_t debug_menu_impall = {
  .id = MENU_ID_DBG_IMPALL,
  .description = "Import all parameters",
  .handler = importAllParameters,
  .parent_id = MENU_ID_DBG,
  .children_ids = NULL,
  .num_children = 0,
  .access_level = 0,
  .parameters = &debug_menu_impall_param
};


/* Exported function definitions ---------------------------------------------*/

void COMM_RegisterDebugMenu(void)
{
  bool ret = MenuSystem_RegisterMenu(&debug_menu) && MenuSystem_RegisterMenu(&debug_menu_gpio) &&
             MenuSystem_RegisterMenu(&debug_menu_set_led) && MenuSystem_RegisterMenu(&debug_menu_print) &&
             MenuSystem_RegisterMenu(&debug_menu_noise) && MenuSystem_RegisterMenu(&debug_menu_temp) &&
             MenuSystem_RegisterMenu(&debug_menu_err) && MenuSystem_RegisterMenu(&debug_menu_pwr) &&
             MenuSystem_RegisterMenu(&debug_menu_dfu) && MenuSystem_RegisterMenu(&debug_menu_reset) &&
             MenuSystem_RegisterMenu(&debug_menu_noise_f) && MenuSystem_RegisterMenu(&debug_menu_noise_level) &&
             MenuSystem_RegisterMenu(&debug_menu_deep_sleep) && MenuSystem_RegisterMenu(&debug_menu_expall) &&
             MenuSystem_RegisterMenu(&debug_menu_impall);

  if (ret == false) REGISTER_ERROR(ERROR_MENU_REGISTRATION);
}

/* Private function definitions ----------------------------------------------*/

// TODO: implement
static void getGpioStatus(FunctionContext_t* context)
{
  COMMLoops_NotImplemented(context);
}

static void setLedColourHandler(FunctionContext_t* context)
{
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

          LED_ManualOverride(red, green, blue);

          context->state->state = PARAM_STATE_COMPLETE;
        }
        break;
      default:
        context->state->state = PARAM_STATE_COMPLETE;
        break;
    }
  } while (old_state > context->state->state); // Continues looping if the state has regressed
}

static void printWaveformHandler(FunctionContext_t* context)
{
  if (print_event_handle == NULL) {
    return;
  }

  osEventFlagsSet(print_event_handle, MESS_PRINT_WAVEFORM);

  COMM_TransmitData("\r\nThe next waveform will be printed. This function should "
                    "only be used with a script.", CALC_LEN, COMM_USB);

  context->state->state = PARAM_STATE_COMPLETE;
}

static void dumpAdcData(FunctionContext_t* context)
{
  if (print_event_handle == NULL) {
    return;
  }

  osEventFlagsSet(print_event_handle, MESS_PRINT_REQUEST);
  uint32_t flags;

  // No watchdog to check for whether the expected response never came
  do {
    // Prevents accidentally corrupting adc dump data
    flags = osEventFlagsWait(print_event_handle, MESS_PRINT_COMPLETE, osFlagsNoClear, osWaitForever);
    osDelay(1);
  } while ((flags & MESS_PRINT_COMPLETE) != MESS_PRINT_COMPLETE);

  osEventFlagsClear(print_event_handle, MESS_PRINT_COMPLETE);

  context->state->state = PARAM_STATE_COMPLETE;
}

static void noiseSpectralAnalysis(FunctionContext_t* context)
{
  if (print_event_handle == NULL) {
    return;
  }

  osEventFlagsSet(print_event_handle, MESS_INPUT_FFT);
  uint32_t flags;

  do {
    // Prevents accidentally corrupting frequency analysis data
    flags = osEventFlagsWait(print_event_handle, MESS_PRINT_COMPLETE, osFlagsNoClear, osWaitForever);
    osDelay(1);
  } while ((flags & MESS_PRINT_COMPLETE) != MESS_PRINT_COMPLETE);

  osEventFlagsClear(print_event_handle, MESS_PRINT_COMPLETE);

  context->state->state = PARAM_STATE_COMPLETE;
}

static void printCurrentTemp(FunctionContext_t* context)
{
  float temp = Temperature_GetCurrentTj();

  sprintf((char*) context->output_buffer, "\r\nCurrent temperature: %.2f C\r\n",
          temp);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

static void printErrorLog(FunctionContext_t* context)
{
  ErrorLog_PrintLog(context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

static void printCurrentPowerConsumption(FunctionContext_t* context)
{
  float power = Power_LatestPower();

  sprintf((char*) context->output_buffer, "\r\nLatest power reading: %.3f W\r\n",
          power);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

static void printBackgroundNoise(FunctionContext_t* context)
{
  if (BackgroundNoise_Ready() == false) {
    COMM_TransmitData("\r\nBackground noise not available yet\r\n", CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  float background_noise = BackgroundNoise_GetNsd();

  sprintf((char*) context->output_buffer, "\r\nBackground noise: %.0f nV/sqrt(Hz)\r\n", background_noise);
  COMM_TransmitData(context->output_buffer, CALC_LEN, context->comm_interface);
  context->state->state = PARAM_STATE_COMPLETE;
}

static void resetSavedValues(FunctionContext_t* context)
{
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

          // Give time to print USB message (not needed in practice)
          osDelay(10);

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

// TODO: add confirmation similar to above
static void enterDfuMode(FunctionContext_t* context)
{
  __HAL_RCC_BKPRAM_CLK_ENABLE();

  HAL_PWR_EnableBkUpAccess();

  if (HAL_PWREx_EnableBkUpReg() != HAL_OK) {
    COMM_TransmitData("Could not set backup flag. Aborted action\r\n", CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }
  // write magic number to magic address. See startup code for corresponding check
  *((uint32_t*) 0x38800000) = MAGIC_RESET_NUMBER;
  volatile uint32_t check_value = *((uint32_t*) 0x38800000);

  if (check_value != MAGIC_RESET_NUMBER) {
    COMM_TransmitData("Error setting DFU flag. Aborting action\r\n", CALC_LEN, context->comm_interface);
    context->state->state = PARAM_STATE_COMPLETE;
    return;
  }

  osDelay(10);

  NVIC_SystemReset();
}

// TODO: add confirmation
static void deepSleep(FunctionContext_t* context)
{
  osEventFlagsSet(sleep_events, SLEEP_REQUEST_DEEP);

  context->state->state = PARAM_STATE_COMPLETE;
}

static void exportAllParameters(FunctionContext_t* context)
{
  ImportExport_ExportAllParameters(context);
}

static void importAllParameters(FunctionContext_t* context)
{
  ImportExport_ImportAllParameters(context);
}
