/*
 * sleep_deep.h
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

#ifndef SLEEP_SLEEP_DEEP_H_
#define SLEEP_SLEEP_DEEP_H_

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
 * @brief Enters a deep sleep where all peripherals are disabled and the device
 * must be woken up by an interrupt
 */
void SleepDeep_Enter();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SLEEP_DEEP_H_ */
