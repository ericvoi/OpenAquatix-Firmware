/*
 * mess_error_correction.h
 *
 *  Created on: Apr 30, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_ERROR_CORRECTION_H_
#define MESS_MESS_ERROR_CORRECTION_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool ErrorCorrection_AddCorrection(BitMessage_t* bit_msg, const DspConfig_t* cfg);
bool ErrorCorrection_CheckCorrection(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble, bool* error_detected, bool* error_corrected);
uint16_t ErrorCorrection_GetLength(const uint16_t length, const ErrorCorrectionMethod_t method);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_ERROR_CORRECTION_H_ */
