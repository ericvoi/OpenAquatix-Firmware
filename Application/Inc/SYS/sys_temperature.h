/*
 * sys_temperature.h
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
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

/**
 * @brief Initializes analog temeprature sensor module
 * 
 * Sets up calibration factors
 * 
 * @return true 
 */
bool Temperature_Init(void);

/**
 * @brief Starts conversion in interrupt mode. Only call from sys_sensor_timer
 * 
 * @return true if ADC call successful, false otherwise
 * 
 * @see SensorTimer_Tick
 */
bool Temperature_TriggerTjConversion(void);

/**
 * @brief Reads the temperature ADC and adds to ring buffer
 * 
 * @see Temperature_TriggerTjConversion()
 */
void Temperature_AddTjValue(void);

/**
 * @brief Processes unprocessed temperature data in temperature buffer
 * 
 * Performs calculations for average temperature and peak temperature
 * 
 * @return true 
 */
bool Temperature_Process(void);

/**
 * @brief Average uC junction/die temperature since reset
 * 
 * @return floating point average uC junction/die temperature in C
 */
float Temperature_GetAverageTj(void);

/**
 * @brief Current uC junction/die temperature
 * 
 * @return floating point current uC junction/die temperature in C
 */
float Temperature_GetCurrentTj(void);

/**
 * @brief Peak uC junction/die temperature since reset
 * 
 * @return floating point peak uC junction/die temperature in C
 */
float Temperature_GetPeakTj(void);

/**
 * @brief Average ambient temperature since reset
 * 
 * @return floating point average ambient temperature in C
 */
float Temperature_GetAverageTa(void);

/**
 * @brief Current ambient temperature
 * 
 * @return floating point average ambient temperature in C
 */
float Temperature_GetCurrentTa(void);

/**
 * @brief Peak ambient temperature since reset
 * 
 * @return floating point peak ambient temperature in C
 */
float Temperature_GetPeakTa(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_TEMPERATURE_H_ */
