/*
 * INA219-driver.h
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

bool INA_Init(void);
bool INA_RegisterBuffer(InaPowerValues_t* buf, uint16_t buf_len, uint16_t* buf_head);
bool INA_Read(void);
void INA_TxComplete(void);
void INA_RxComplete(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __INA219_DRIVER_H_ */
