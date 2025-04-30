/*
 * dac_main.h
 *
 *  Created on: Apr 28, 2025
 *      Author: ericv
 */

#ifndef DAC_DAC_MAIN_H_
#define DAC_DAC_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define DAC_FILL_FIRST_HALF   (1 << 0)
#define DAC_FILL_LAST_HALF    (1 << 1)


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void DAC_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DAC_DAC_MAIN_H_ */
