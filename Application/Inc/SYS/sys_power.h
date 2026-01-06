/*
 * sys_power.h
 *
 *  Created on: Jan 31, 2025
 *      Author: cjcockrall
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef __SYS_POWER_H_
#define __SYS_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include "INA219-driver.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define INA_PERIOD_MS 1 // Power reading every 1ms


/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool INA219_System_Init(void);
void INA219_Timer_Callback(void);
void INA219_ReadComplete_Callback(bool success);
float Power_GetRecentAverage(uint8_t numsamples);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_INA219_H_ */