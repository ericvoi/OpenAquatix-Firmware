/*
 * sys_led.c
 *
 *  Created on: Apr 25, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_led.h"

#include "mess_main.h"

#include "cfg_defaults.h"
#include "cfg_parameters.h"

#include "ws2812b-driver.h"
#include "power_leds.h"
#include "cmsis_os.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define RGB_RED       255,0,  0
#define RGB_GREEN     0,  255,0
#define RGB_BLUE      0,  0,  255
#define RGB_WHITE     255,255,255
#define RGB_YELLOW    255,255,0
#define RGB_MAGENTA   255,0,  255
#define RGB_CYAN      0,  255,255
#define RGB_OFF       0,  0,  0
#define RGB_ORANGE    255,100,0

#define LISTENING_COLOUR      RGB_GREEN
#define DRIVING_COLOUR        RGB_BLUE
#define PROCESSING_COLOUR     RGB_MAGENTA
#define CHANGING_COLOUR       RGB_OFF

#define ERROR_COLOUR          RGB_RED
#define ABORT_COLOUR          RGB_ORANGE
#define WARNING_COLOUR        RGB_YELLOW

#define OVERRIDE_DURATION_MS 10000

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint8_t led_brightness = DEFAULT_LED_BRIGHTNESS;
static bool led_enable = DEFAULT_LED_STATE;

static bool manual_override = false;
static uint8_t manual_r;
static uint8_t manual_g;
static uint8_t manual_b;
static uint32_t manual_override_start_time;

static const uint8_t power_led_state = 0x0F;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void LED_Update()
{
  PWRLED_Update(power_led_state); // TODO: change to be updatable

  uint8_t brightness = led_enable ? led_brightness : 0;
  if (manual_override == true) {
    uint32_t current_ticks = osKernelGetTickCount();
    if (current_ticks - manual_override_start_time > OVERRIDE_DURATION_MS) {
      manual_override = false;
    }
    else {
      Ws2812b_SetColour(manual_r, manual_g, manual_b);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    }
  }

  OpenAquatixStatus_t status = Error_GetStatus();
  switch (status) {
    case OA_STATUS_ERROR:
      Ws2812b_SetColour(ERROR_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case OA_STATUS_ABORT:
      Ws2812b_SetColour(ABORT_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case OA_STATUS_WARN:
      Ws2812b_SetColour(WARNING_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case OA_STATUS_OK:
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }

  // No warnings or errors -> show MESS task state
  ProcessingState_t state = MESS_GetState();

  switch (state) {
    case HIL_CALIBRATION:
    case LISTENING:
      // set led to default if no warnings
      Ws2812b_SetColour(LISTENING_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case DRIVING_TRANSDUCER:
      Ws2812b_SetColour(DRIVING_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case PROCESSING:
      Ws2812b_SetColour(PROCESSING_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    case CHANGING:
      Ws2812b_SetColour(CHANGING_COLOUR);
      if (Ws2812b_Update(brightness) == false) REGISTER_ERROR(ERROR_RGB_LED);
      return;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void LED_ManualOverride(uint8_t r, uint8_t g, uint8_t b)
{
  manual_override = true;

  manual_r = r;
  manual_b = b;
  manual_g = g;

  manual_override_start_time = osKernelGetTickCount();
}

void LED_RegisterParams()
{
  uint32_t min = MIN_LED_BRIGHTNESS;
  uint32_t max = MAX_LED_BRIGHTNESS;
  if (Param_Register(PARAM_LED_BRIGHTNESS, "RGB LED brightness", PARAM_TYPE_UINT8,
                     &led_brightness, sizeof(uint8_t), &min, &max, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min = MIN_LED_STATE;
  max = MAX_LED_STATE;
  if (Param_Register(PARAM_LED_ENABLE, "the onboard RGB LED", PARAM_TYPE_UINT8,
                     &led_enable, sizeof(bool), &min, &max, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
}

/* Private function definitions ----------------------------------------------*/
