/*
 * sys_sensor_timer.h
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 */

#ifndef SYS_SYS_SENSOR_TIMER_H_
#define SYS_SYS_SENSOR_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define SENSOR_TIMER_SOURCE             htim16

#define SENSOR_TIMER_TICK_RATE_HZ       1000
// Trigger a temeprature sensor reading every 200ms
#define TEMPERATURE_SENSOR_PERIOD_MS    200

extern TIM_HandleTypeDef SENSOR_TIMER_SOURCE;

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/
/**
 * @brief Initializes timer for the sensor timer which controls when the
 * temperature sensor and power monitor are triggered.
 * 
 * Starts the timer that updates the tick count
 * 
 * @return true if initialization of timer is successful, false otherwise
 */
bool SensorTimer_Init(void);

/**
 * @brief Updates tick count. If the tick count is a multiple of a value then
 * either the temperature reading or power reading is taken
 */
void SensorTimer_Tick(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_SENSOR_TIMER_H_ */
