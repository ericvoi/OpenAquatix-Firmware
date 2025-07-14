/*
 * uam_math.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_UAM_MATH_H_
#define COMMON_UTILS_UAM_MATH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Fast version of sinf that uses CORDIC
 * 
 * @param input Angle to calculate sine of (prescaled by 1/pi)
 * @return float sine(input)
 * 
 * @note The input MUST be prescaled by 1/pi
 */
float uam_sinf(float input);

/**
 * @brief Fast version of cosf that uses CORDIC
 * 
 * @param input Angle to calculate the cosine of (prescaled by 1/pi)
 * @return float cosine(input)
 * 
 * @note The input MUST be prescaled by 1/pi
 */
float uam_cosf(float input);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_UAM_MATH_H_ */
