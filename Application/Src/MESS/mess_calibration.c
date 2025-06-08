/*
 * mess_calibration.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_calibration.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint32_t mod_cal_lower_freq = DEFAULT_MOD_CAL_LOWER_FREQ;
static uint32_t mod_cal_upper_freq = DEFAULT_MOD_CAL_UPPER_FREQ;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Calibrate_RegisterParams()
{
  uint32_t min = MIN_MOD_CAL_LOWER_FREQ;
  uint32_t max = MAX_MOD_CAL_LOWER_FREQ;
  if (Param_Register(PARAM_MOD_CAL_LOWER_FREQ, "lower calibration frequency", PARAM_TYPE_UINT32,
                     &mod_cal_lower_freq, sizeof(uint32_t), &min, &max, NULL) == false) {
    return false;
  }

  min = MIN_MOD_CAL_UPPER_FREQ;
  max = MAX_MOD_CAL_UPPER_FREQ;
  if (Param_Register(PARAM_MOD_CAL_UPPER_FREQ, "upper calibration frequency", PARAM_TYPE_UINT32,
                     &mod_cal_upper_freq, sizeof(uint32_t), &min, &max, NULL) == false) {
    return false;
  }
  return true;
}

/* Private function definitions ----------------------------------------------*/
