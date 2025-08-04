/*
 * sleep_manager.h
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

#ifndef SLEEP_SLEEP_MANAGER_H_
#define SLEEP_SLEEP_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "sleep/sleep_config.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Checks if a sleep mode needs to be entered, and, if so, disables the
 * scheduler and enters into the corresponding loop
 */
void SleepManager_Enter();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SLEEP_MANAGER_H_ */
