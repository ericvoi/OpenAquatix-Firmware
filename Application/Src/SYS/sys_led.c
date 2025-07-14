/*
 * sys_led.c
 *
 *  Created on: Apr 25, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_led.h"
#include "sys_error.h"

#include "mess_main.h"

#include "cfg_defaults.h"
#include "cfg_parameters.h"

#include "WS2812b-driver.h"
#include "cmsis_os.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define RGB_RED       255,0,0
#define RGB_GREEN     0,255,0
#define RGB_BLUE      0,0,255
#define RGB_WHITE     255,255,255
#define RGB_YELLOW    255,255,0
#define RGB_MAGENTA   255,0,255
#define RGB_CYAN      0,255,255
#define RGB_OFF       0,0,0

#define LISTENING_COLOUR      RGB_GREEN
#define DRIVING_COLOUR        RGB_BLUE
#define PROCESSING_COLOUR     RGB_MAGENTA
#define CHANGING_COLOUR       RGB_OFF

#define ERROR_COLOUR          RGB_RED
#define WARNING_COLOUR        RGB_YELLOW // TODO: add warning system

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

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool LED_Update()
{
  uint8_t brightness = led_enable ? led_brightness : 0;
  if (manual_override == true) {
    uint32_t current_ticks = osKernelGetTickCount();
    if (current_ticks - manual_override_start_time > OVERRIDE_DURATION_MS) {
      manual_override = false;
    }
    else {
      WS_SetColour(manual_r, manual_g, manual_b);
      WS_Update(brightness);
      return true;
    }
  }

  // check for errors. If errors exist, set to red
  if (Error_Exists()) {
    WS_SetColour(ERROR_COLOUR);
    WS_Update(brightness);
    return true;
  }

  // check mess task state
  ProcessingState_t state = MESS_GetState();

  switch (state) {
    case LISTENING:
      // set led to default if no warnings
      WS_SetColour(LISTENING_COLOUR);
      WS_Update(brightness);
      return true;
    case DRIVING_TRANSDUCER:
      WS_SetColour(DRIVING_COLOUR);
      WS_Update(brightness);
      return true;
    case PROCESSING:
      WS_SetColour(PROCESSING_COLOUR);
      WS_Update(brightness);
      return true;
    case CHANGING:
      WS_SetColour(CHANGING_COLOUR);
      WS_Update(brightness);
      return true;
    default:
      return false;
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

bool LED_RegisterParams()
{
  uint32_t min = MIN_LED_BRIGHTNESS;
  uint32_t max = MAX_LED_BRIGHTNESS;
  if (Param_Register(PARAM_LED_BRIGHTNESS, "RGB LED brightness", PARAM_TYPE_UINT8,
                     &led_brightness, sizeof(uint8_t), &min, &max, NULL) == false) {
    return false;
  }

  min = MIN_LED_STATE;
  max = MAX_LED_STATE;
  if (Param_Register(PARAM_LED_ENABLE, "the onboard RGB LED", PARAM_TYPE_UINT8,
                     &led_enable, sizeof(bool), &min, &max, NULL) == false) {
    return false;
  }

  return true;
}

/* Private function definitions ----------------------------------------------*/
