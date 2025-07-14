/*
 * uam_math.c
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "uam_math.h"
#include "arm_math.h"
#include "stm32h7xx.h"
#include "stm32h7xx_ll_cordic.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

float uam_sinf(float input)
{
  LL_CORDIC_SetFunction(CORDIC, LL_CORDIC_FUNCTION_SINE);
  q31_t temp;
  arm_float_to_q31(&input, &temp, 1);
  LL_CORDIC_WriteData(CORDIC, temp);
  temp = LL_CORDIC_ReadData(CORDIC);
  arm_q31_to_float(&temp, &input, 1);
  return input;
}

float uam_cosf(float input)
{
  LL_CORDIC_SetFunction(CORDIC, LL_CORDIC_FUNCTION_COSINE);
  q31_t temp;
  arm_float_to_q31(&input, &temp, 1);
  LL_CORDIC_WriteData(CORDIC, temp);
  temp = LL_CORDIC_ReadData(CORDIC);
  arm_q31_to_float(&temp, &input, 1);
  return input;
}

/* Private function definitions ----------------------------------------------*/
