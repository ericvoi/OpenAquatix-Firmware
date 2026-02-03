/*
 * tpa32xx-driver.h
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef AFE_INTERNAL_TPA32XX_DRIVER_H_
#define AFE_INTERNAL_TPA32XX_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  TPA_OFF,
  TPA_MUTED,
  TPA_ACTIVE
} TpaState_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Enables the TPA32xx power amplifier
 * 
 * Turns on the respective power supply unmutes, and enables the filter
 * 
 * @return true if successfully turned on, false if either error with power,
 * unmuting, or enabling the filter
 */
bool TPA_Enable(void);

/**
 * @brief Mutes the TPA power amplifier by asserting reset and disabling filter
 * 
 * @return true if successful, false if preconditions not met
 */
bool TPA_Mute(void);

/**
 * @brief Unmutes the power amplifier by deasserting reset and enabling filter
 * 
 * @return true if successful, false if preconditions not met
 */
bool TPA_Unmute(void);

/**
 * @brief Shuts down the TPA power amplifier
 * 
 * Disables the power supply entirely, asserts reset, and disables the filter
 * 
 * @return true if successful, false otherwise
 */
bool TPA_Shutdown(void);

/**
 * @brief Current state of TPA power amplifier
 * 
 * @return TpaState_t Either TPA_OFF, TPA_MUTED, or TPA_ACTIVE
 * 
 * @note Does not indicate anything about the input filter status
 * 
 * @see TPA_Ready()
 */
TpaState_t TPA_State(void);

/**
 * @brief Whether the power amplifier is ready for use
 * 
 * @return true if filter on, TPA powered, and not muted, false otherwise
 * 
 * @see TPA_State()
 */
bool TPA_Ready(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_INTERNAL_TPA32XX_DRIVER_H_ */
