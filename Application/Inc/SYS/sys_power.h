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
#include "ina219-driver.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define INA_PERIOD_MS 1 // Power reading every 1ms


/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Power_Init(void);
void Power_Process(void);
float Power_MinPower(void);
float Power_MaxPower(void);
float Power_AveragePower(void);
float Power_RecentAveragePower(uint8_t numsamples);
float Power_MinVoltage(void);
float Power_MaxVoltage(void);
float Power_AverageVoltage(void);
float Power_MinCurrent(void);
float Power_MaxCurrent(void);
float Power_AverageCurrent(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_INA219_H_ */