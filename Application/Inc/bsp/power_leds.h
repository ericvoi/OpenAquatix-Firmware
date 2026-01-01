/*
 * power_leds.h
 *
 *  Created on: Dec 28, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef DRIVERS_POWER_LEDS_H_
#define DRIVERS_POWER_LEDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  LED_3V3 = 1 << 0,
  LED_3V3A = 1 << 1,
  LED_30V = 1 << 2,
  LED_BATT = 1 << 3,
} LedStates_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Updates the state of each power LED: 3.3V, 3.3VA, 30V, and BATT
 * 
 * The corresponding red LED will only turn on when its state is 1 and the
 * power rail is present. Use LedStates_t to create the states passed to this
 * function
 * 
 * @param states LED states comprised of LedStates_t;
 */
void PWRLED_Update(uint8_t states);

/**
 * @brief Returns most recent state of PWR LED configuration
 * 
 * @return uint8_t latest states passed to PWRLED_Update
 */
uint8_t PWRLED_CurrentState(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_POWER_LEDS_H_ */
