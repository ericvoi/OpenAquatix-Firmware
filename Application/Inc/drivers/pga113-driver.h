/*
 * PGA113-driver.h
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef __PGA113_DRIVER_H_
#define __PGA113_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "cfg_parameters.h"
#include <stdbool.h>
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define PGA_GAIN_TABLE(X) \
  X(PGA_GAIN_1, "1") \
  X(PGA_GAIN_2, "2") \
  X(PGA_GAIN_5, "5") \
  X(PGA_GAIN_10, "10") \
  X(PGA_GAIN_20, "20") \
  X(PGA_GAIN_50, "50") \
  X(PGA_GAIN_100, "100") \
  X(PGA_GAIN_200, "200")

DECLARE_ENUM(PGA_GAIN_TABLE, PGA_NUM_CODES, PgaGain_t)

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the PGA113 driver by setting flags
 */
void Pga113_Init();

/**
 * @brief Sets the gain of the PGA113 and updates the device instantly if available
 * 
 * @param gain New gain (scope gains)
 */
void Pga113_SetGain(PgaGain_t gain);

/**
 * @brief Reads from the PGA113 and updates the rx buffer when received.
 * Used for debugging SPI connection
 * 
 * @note On failure, cause PGA subsystem to be reset
 */
void Pga113_Read();

/**
 * @brief Shuts down the PGA113. This makes the output undefined and not tied
 * to the input. Useful for deep sleep modes. Iq ~= 1.1mA
 * 
 * @note On failure, cause PGA subsystem to be reset
 */
void Pga113_Shutdown();

/**
 * @brief Enables the PGA113 after being shutdown. Re enables output PGA signal
 * 
 * @note On failure, cause PGA subsystem to be reset
 */ 
void Pga113_Enable();

/**
 * @brief Gets the last gain of the PGA module.
 * 
 * @return PgaGain_t Last gain that the PGA113 was updated with
 */
PgaGain_t Pga113_GetGain();

#ifdef __cplusplus
}
#endif

#endif /* __PGA113_DRIVER_H_ */
