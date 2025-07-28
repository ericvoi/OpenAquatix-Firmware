/*
 * sys_sleep.h
 *
 *  Created on: Jul 27, 2025
 *      Author: ericv
 */

#ifndef SYS_SYS_SLEEP_H_
#define SYS_SYS_SLEEP_H_

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

#define SLEEP_REQUEST_LIGHT   (1 << 0)
#define SLEEP_REQUEST_DEEP    (1 << 1)
#define SLEEP_WAKEUP_MESS     (1 << 2)

/* Exported functions prototypes ---------------------------------------------*/

bool Sleep_Init();
void Sleep_Check();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_SLEEP_H_ */
