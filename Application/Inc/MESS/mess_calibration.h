/*
 * mess_calibration.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_CALIBRATION_H_
#define MESS_MESS_CALIBRATION_H_

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
 * @brief Registers parameters for the calibration module
 * 
 * @note Logs an error in the event of a failure
 */
void Calibrate_RegisterParams(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_CALIBRATION_H_ */
