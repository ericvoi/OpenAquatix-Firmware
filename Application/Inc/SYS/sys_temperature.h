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

/**
 * @brief Initializes analog temeprature sensor module
 * 
 * Sets up calibration factors
 * 
 * @return true 
 */
bool Temperature_Init();

/**
 * @brief Starts conversion in interrupt mode. Only call from sys_sensor_timer
 * 
 * @return true if ADC call successful, false otherwise
 * 
 * @see SensorTimer_Tick
 */
bool Temperature_TriggerConversion();

/**
 * @brief Reads the temperature ADC and adds to ring buffer
 * 
 * @see Temperature_TriggerConversion()
 */
void Temperature_AddValue();

/**
 * @brief Processes unprocessed temperature data in temperature buffer
 * 
 * Performs calculations for average temperature and peak temperature
 * 
 * @return true 
 */
bool Temperature_Process();

/**
 * @brief Average temperature since reset
 * 
 * @return floating point average temperature in C
 */
float Temperature_GetAverage();

/**
 * @brief Current temperature
 * 
 * @return floating point current temperature in C
 */
float Temperature_GetCurrent();

/**
 * @brief Peak temperature since reset
 * 
 * @return floating point peak temperature in C
 */
float Temperature_GetPeak();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_TEMPERATURE_H_ */
