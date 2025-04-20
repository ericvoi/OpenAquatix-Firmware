/*
 * sys_main.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_main.h"
#include "main.h"
#include "sys_error.h"
#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "WS2812b-driver.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t led_brightness = DEFAULT_LED_BRIGHTNESS;
static bool led_enable = DEFAULT_LED_STATE;

/* Private function prototypes -----------------------------------------------*/

bool registerSysParam();

/* Exported function definitions ---------------------------------------------*/

void SYS_StartTask(void* argument)
{
  (void)(argument);
  if (Param_RegisterTask(SYS_TASK, "SYS") == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (registerSysParam() == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  if (Param_TaskRegistrationComplete(SYS_TASK) == false) {
    Error_Routine(ERROR_SYS_INIT);
  }

  CFG_WaitLoadComplete();

  for (;;) {
    WS_Update();
    osDelay(1000);
  }
}

/* Private function definitions ----------------------------------------------*/

bool registerSysParam()
{
  uint32_t min = MIN_LED_BRIGHTNESS;
  uint32_t max = MAX_LED_BRIGHTNESS;
  if (Param_Register(PARAM_LED_BRIGHTNESS, "RGB LED brightness", PARAM_TYPE_UINT16,
                     &led_brightness, sizeof(uint16_t), &min, &max) == false) {
    return false;
  }

  min = MIN_LED_STATE;
  max = MAX_LED_STATE;
  if (Param_Register(PARAM_LED_ENABLE, "the onboard RGB LED", PARAM_TYPE_UINT8,
                     &led_enable, sizeof(bool), &min, &max) == false) {
    return false;
  }

  return true;
}
