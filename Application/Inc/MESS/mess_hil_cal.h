/*
 * mess_hil_cal.h
 *
 *  Created on: Apr 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_HIL_CAL_H_
#define MESS_MESS_HIL_CAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "hil_manager.h"
#include "mess_dsp_config.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Performs calibration for OpenCREST HIL testing
 * 
 * @param cfg Modem parameter configuration
 * 
 * @note Does not let calling task run until complete (~700 ms)
 * @note Does not cause the modem to enter HIL mode
 */
void HilCal_Perform(const DspConfig_t* cfg);

/**
 * @brief Copies calibration information to pointer
 * 
 * @param cal_packet Where to copy calibration data to
 * @return true if successful, false otherwise
 */
bool HilCal_Get(HilCalibrationPacket_t* cal_packet);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_HIL_CAL_H_ */
