/*
 * PGA113-driver.h
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

#ifndef __PGA113_DRIVER_H_
#define __PGA113_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  PGA_GAIN_1,
  PGA_GAIN_2,
  PGA_GAIN_5,
  PGA_GAIN_10,
  PGA_GAIN_20,
  PGA_GAIN_50,
  PGA_GAIN_100,
  PGA_GAIN_200,

  PGA_NUM_CODES
} PgaGain_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the PGA113 driver by setting flags
 * 
 * @return true always
 */
bool Pga113_Init();

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
 * @return true if successfully set DMA in HAL, false otherwise 
 */
bool Pga113_Read();

/**
 * @brief Shuts down the PGA113. This makes the output undefined and not tied
 * to the input. Useful for deep sleep modes. Iq ~= 1.1mA
 * 
 * @return true if successfully sent shutdown command, false otherwise
 */
bool Pga113_Shutdown();

/**
 * @brief Enables the PGA113 after being shutdown. Re enables output PGA signal
 * 
 * @return true if successfully sent command to disable shutdown, false otherwise
 */
bool Pga113_Enable();

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
