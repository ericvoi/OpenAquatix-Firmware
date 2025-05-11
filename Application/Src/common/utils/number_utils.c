/*
 * number_utils.c
 *
 *  Created on: May 1, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "number_utils.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool NumberUtils_IsPowerOf2(uint16_t num)
{
  return (num != 0) && ((num & (num - 1)) == 0);
}

/* Private function definitions ----------------------------------------------*/
