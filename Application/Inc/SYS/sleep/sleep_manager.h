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

void SleepManager_Enter();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SLEEP_MANAGER_H_ */
