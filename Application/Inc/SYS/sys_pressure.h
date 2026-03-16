/*
 * sys_pressure.h
 *
 *  Created on: Jan 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef SYS_SYS_PRESSURE_H_
#define SYS_SYS_PRESSURE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Sets up pressure module by initializing buffer and registering it
 */
void Pressure_Init(void);

/**
 * @brief Processes all unprocessed raw temperature readings and updates most
 * recent pressure
 */
void Pressure_Process(void);

/**
 * @brief The most recent reading from the LPS22HH
 * 
 * @return float Pressure in hPa
 */
float Pressure_GetCurrent(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_PRESSURE_H_ */
