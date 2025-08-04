/*
 * sleep_config.h
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

#ifndef SLEEP_SLEEP_CONFIG_H_
#define SLEEP_SLEEP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  SLEEP_NONE,
  SLEEP_LIGHT,
  SLEEP_DEEP
} SleepStates_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_SLEEP_CONFIG_H_ */
