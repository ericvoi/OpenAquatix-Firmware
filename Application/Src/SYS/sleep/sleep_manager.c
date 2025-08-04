/*
 * sleep_manager.c
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sleep/sleep_manager.h"
#include "sleep/sleep_config.h"
#include "sleep/sleep_deep.h"
#include "sys_main.h"
#include "cmsis_os.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern osEventFlagsId_t sleep_events;

/* Private function prototypes -----------------------------------------------*/

static SleepStates_t getSleepMode();

/* Exported function definitions ---------------------------------------------*/

void SleepManager_Enter()
{
  SleepStates_t sleep_state = getSleepMode();
  switch (sleep_state) {
    case SLEEP_NONE:
      return;
    case SLEEP_LIGHT:
      return;
    case SLEEP_DEEP:
      SleepDeep_Enter();
    default:
      return;
  }
}

/* Private function definitions ----------------------------------------------*/

SleepStates_t getSleepMode()
{
  if (sleep_events == NULL) {
    return SLEEP_NONE;
  }

  uint32_t events = osEventFlagsGet(sleep_events);

  if (events & SLEEP_REQUEST_LIGHT) {
    return SLEEP_LIGHT;
  }

  if (events & SLEEP_REQUEST_DEEP) {
    return SLEEP_DEEP;
  }

  return SLEEP_NONE;
}
