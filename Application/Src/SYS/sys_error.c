/*
 * sys_error.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_error.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static bool error = false;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void Error_Routine(ErrorCodes_t error_code)
{
  switch (error_code) {
    case ERROR_CFG_INIT:
    case ERROR_COMM_INIT:
    case ERROR_MESS_INIT:
    case ERROR_SYS_INIT:
    case ERROR_MESS_PROCESSING:
    case ERROR_DAC_INIT:
    case ERROR_DAC_PROCESSING:
    case ERROR_MESS_DAC_RESOURCE:
      break;
    default:
      break;
  }

  error = true;
}

bool Error_Exists()
{
  return error;
}

/* Private function definitions ----------------------------------------------*/
