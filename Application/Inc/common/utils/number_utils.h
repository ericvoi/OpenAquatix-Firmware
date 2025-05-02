/*
 * number_utils.h
 *
 *  Created on: May 1, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_NUMBER_UTILS_H_
#define COMMON_UTILS_NUMBER_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief checks if number is a power of 2
 *
 * @param num Number to check
 *
 * @return true if power of 2, false otherwise
 */
bool NumberUtils_IsPowerOf2(uint16_t num);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_NUMBER_UTILS_H_ */
