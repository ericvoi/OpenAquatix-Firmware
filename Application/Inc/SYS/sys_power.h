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



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the power module by initializing the TPA and registering the buffer
 * 
 * @return true if successful initialization, false otherwise
 */
bool Power_Init(void);

/**
 * @brief Processes new datapoints for averages, mins, and maxes
 */
void Power_Process(void);

/**
 * @brief Min power since boot
 * 
 * @return float Min power since boot (W)
 */
float Power_MinPower(void);

/**
 * @brief Max power since boot
 * 
 * @return float Max power since boot (W)
 */
float Power_MaxPower(void);

/**
 * @brief Average power since boot
 * 
 * @return float Average power since boot (W)
 */
float Power_AveragePower(void);

/**
 * @brief Latest power reading
 * 
 * @return float Latest power reading (W)
 */
float Power_LatestPower(void);

/**
 * @brief Averages a small window of power readings for calibration
 * 
 * @param numsamples The number of samples in the average
 * @return float Recent power average (W)
 */
float Power_RecentAveragePower(uint8_t numsamples);

/**
 * @brief Min voltage since boot
 * 
 * @return float Min voltage since boot (V)
 */
float Power_MinVoltage(void);

/**
 * @brief Max voltage since boot
 * 
 * @return float Max voltage since boot (V)
 */
float Power_MaxVoltage(void);

/**
 * @brief Average voltage since boot
 * 
 * @return float Average voltage since boot (V)
 */
float Power_AverageVoltage(void);

/**
 * @brief Latest voltage reading
 * 
 * @return float Latest voltage reading (V)
 */
float Power_LatestVoltage(void);

/**
 * @brief Min current since boot
 * 
 * @return float Min current since boot (A)
 */
float Power_MinCurrent(void);

/**
 * @brief Max current since boot
 * 
 * @return float Max current since boot (A)
 */
float Power_MaxCurrent(void);

/**
 * @brief Average current since boot
 * 
 * @return float Average current since boot (A)
 */
float Power_AverageCurrent(void);

/**
 * @brief Latest current reading
 * 
 * @return float Latest current reading (A)
 */
float Power_LatestCurrent(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_INA219_H_ */