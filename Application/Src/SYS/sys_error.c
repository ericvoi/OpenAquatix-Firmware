/*
 * sys_error.c
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_error.h"
#include "WS2812b-driver.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



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
      WS_SetColour(0, 255, 0, 0);
      break;
    default:
      WS_SetColour(0, 0, 0, 255);
      break;
  }
}

/* Private function definitions ----------------------------------------------*/
