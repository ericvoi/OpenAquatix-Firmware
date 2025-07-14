/*
 * sys_led.h
 *
 *  Created on: Apr 25, 2025
 *      Author: ericv
 */

#ifndef SYS_SYS_LED_H_
#define SYS_SYS_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Changes the colour of the LED as needed
 *
 * @return true if successful, false otherwise
 */
bool LED_Update(void);

/**
 * @brief Sets a flag to manually override LED colour for 10s
 *
 * @param r Red colour from 0-255
 * @param g Green colour from 0-255
 * @param b Blue Colour from 0-255
 *
 * @see LED_Update
 */
void LED_ManualOverride(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Registers LED module parameters
 *
 * Register LED brightness and LED enable parameters
 *
 * @return true if successful and false otherwise
 */
bool LED_RegisterParams(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_LED_H_ */
