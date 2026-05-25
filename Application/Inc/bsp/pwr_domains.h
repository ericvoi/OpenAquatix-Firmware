/*
 * pwr_domains.h
 *
 *  Created on: Dec 28, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef DRIVERS_PWR_DOMAINS_H_
#define DRIVERS_PWR_DOMAINS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "stdbool.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  PWR_OFF,            // Voltage rail not powered
  PWR_TRANSITIONING,  // Power transitioning between on and off or vice versa
  PWR_READY,          // Rail stable and ready to use
  PWR_ERROR           // Only applicable to 30V power supply. Returned if Pgood is low
} PowerDomainState_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Turns the analog power domain (+/- 3.3VA) on or off
 * 
 * @param on True to turn on. False to turn off
 * 
 * @note There is a small delay before the power rail settles
 */
void PWR_Analog(bool on);

/**
 * @brief Turns the 30V power domain on or off. Also controls 12V
 * 
 * @param on True to turn on. False to turn off
 * 
 * @note There is a small delay before the power rail settles
 */
void PWR_30V(bool on);

/**
 * @brief Turns the WS2812b power domain on or off
 * 
 * @param on True to turn on. False to turn off
 * 
 * @note There is a small delay before the power rail settles
 */
void PWR_Ws5V(bool on);

/**
 * @brief Current state of the analog power domain
 * 
 * @return PowerDomainState_t State of the domain (off, transitioning, or off)
 */
PowerDomainState_t PWR_StateAnalog(void);

/**
 * @brief Current state of the 30V power domain
 * 
 * @return PowerDomainState_t State of the domain (off, transitioning, off, or error)
 */
PowerDomainState_t PWR_State30V(void);

/**
 * @brief Current state of the Ws2812b power domain
 * 
 * @return PowerDomainState_t State of the domain (off, transitioning, or off)
 */
PowerDomainState_t PWR_StateWs5V(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_PWR_DOMAINS_H_ */
