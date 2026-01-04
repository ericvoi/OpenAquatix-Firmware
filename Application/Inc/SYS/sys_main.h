/*
 * sys_main.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef SYS_SYS_MAIN_H_
#define SYS_SYS_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/

#define SLEEP_REQUEST_LIGHT   (1 << 0)
#define SLEEP_REQUEST_DEEP    (1 << 1)
#define SLEEP_WAKEUP_MESS     (1 << 2)

/* Exported functions prototypes ---------------------------------------------*/

void SYS_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_MAIN_H_ */
