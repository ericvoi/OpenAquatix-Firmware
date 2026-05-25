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
 * @brief Starts the DAC for rx stream feedback output. Also starts timer
 * clocking samples
 * 
 * @note Samples must be ready in the DAC buffer when this function is called
 */
void HilStream_StartDac(void);

/**
 * @brief Stops HIL ADC (DMA only not the timer)
 */
void HilStream_StopAdc(void);

/**
 * @brief Stops HIL DAC (DMA only not the timer)
 */
void HilStream_StopDac(void);

/**
 * @brief Call when in HIL mode and need to add samples from FIFO to DMA buf
 * 
 * @param first_half First half of the DMA buffer needs filling
 */
void HilStream_DacCallback(bool first_half);

/**
 * @brief Call when in HIL mode and need to take samples from DMA buf to FIFO
 * 
 * @param first_half First half of the DMA buffer needs draining
 */
void HilStream_AdcCallback(bool first_half);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_STREAM_H_ */
