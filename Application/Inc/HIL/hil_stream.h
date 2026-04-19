/*
 * hil_stream.h
 *
 *  Created on: Apr 5, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef HIL_HIL_STREAM_H_
#define HIL_HIL_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Starts the HIL ADC for the TX stream. Does not start the timer as
 * this is done when transmitting
 */
void HilStream_StartAdc(void);

/**
 * @brief Starts the DAC for rx stream feedback output
 * 
 */
void HilStream_StartDac(void);
void HilStream_StopAdc(void);
void HilStream_StopDac(void);
void HilStream_DacCallback(bool first_half);
void HilStream_AdcCallback(bool first_half);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_STREAM_H_ */
