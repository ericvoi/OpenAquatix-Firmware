/*
 * mess_feedback.h
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_FEEDBACK_H_
#define MESS_MESS_FEEDBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define FEEDBACK_TEST_DURATION_MS   16

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Feedback_Init();
void Feedback_DumpData();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_FEEDBACK_H_ */
