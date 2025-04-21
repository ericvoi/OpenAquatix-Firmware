/*
 * sys_temperature.h
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 */

#ifndef SYS_SYS_TEMPERATURE_H_
#define SYS_SYS_TEMPERATURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define TEMPERATURE_ADC   hadc3

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Temperature_Init();
bool Temperature_TriggerConversion();
void Temperature_AddValue();
bool Temperature_Process();
float Temperature_GetAverage();
float Temperature_GetCurrent();
float Temperature_GetPeak();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_TEMPERATURE_H_ */
