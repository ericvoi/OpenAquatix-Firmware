/*
 * ina219-driver.h
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef __INA219_DRIVER_H_
#define __INA219_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  float current_A;
  float bus_voltage;
  float power;
  uint32_t timestamp;
} InaPowerValues_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the INA219 power monitor by writing to the configuration
 * register with fixed values (12b, no averaging, continuous)
 * 
 * @return true if initialization succeeded, false otherwise
 * 
 * @note This must be called before using the module
 */
bool INA_Init(void);

/**
 * @brief Registers buffer to store INA power values
 * 
 * @param buf Buffer storing current, voltage, and power values
 * @param buf_len Length of the above buffer
 * @param buf_head Pointer to head of buffer. Updated when there is a new sample
 * 
 * @return true if none of the values are NULL or 0
 * 
 * @note This must be called before using the module
 */
bool INA_RegisterBuffer(InaPowerValues_t* buf, uint16_t buf_len, volatile uint16_t* buf_head);

/**
 * @brief Initiates a read of the bus voltage, and shunt voltage registers on the INA219
 * 
 * @return true if state correct (INA_IDLE) and driver returns HAL_OK, false otherwise
 */
bool INA_StartRead(void);

/**
 * @brief Callback called when memory tx complete
 * 
 * Signals that the INA219 is ready
 */
void INA_TxComplete(void);

/**
 * @brief Callback when memory rx is complete
 * 
 * Calls to the next state in the reading state machine
 */
void INA_RxComplete(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __INA219_DRIVER_H_ */
