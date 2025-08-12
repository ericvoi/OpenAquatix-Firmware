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

/**
 * @brief Adds error correction to a bit message
 * 
 * Error correction for the preamble and the message (data and error detection)
 * is applied separately with potentially different ECC methods
 * 
 * @param bit_msg Bit message to apply ECC to. ECC is applied in place
 * @param cfg Configuration (using ECC methods only)
 * 
 * @return true if successful, false otherwise
 */
bool ErrorCorrection_AddCorrection(BitMessage_t* bit_msg, 
                                   const DspConfig_t* cfg);

/**
 * @brief Decodes an ECC preamble/message
 * 
 * Since ECC is applied separately to the preamble and the message body, the
 * caller must state whether they are performing bit reconstruction on the
 * preamble or the message body.
 * 
 * @param bit_msg Bit message with the error correction coding built in
 * @param cfg Configuration (using ECC methods only)
 * @param is_preamble Flag determining if ECC is to be done on the preamble
 * @param error_detected Pointer to flag indicating error found (potentially updated)
 * @param error_corrected Pointer to flag indicating error corrected (will be updated)
 * 
 * @return true if successfully ran without errors, false otherwise
 * 
 * @note Not all ECC methods can detect errors so error_detected cannot be
 * uninitialized
 */
bool ErrorCorrection_CheckCorrection(BitMessage_t* bit_msg, 
                                     const DspConfig_t* cfg, 
                                     bool is_preamble, 
                                     bool* error_detected, 
                                     bool* error_corrected);

/**
 * @brief Returns length of a sequence with ECC
 * 
 * @param length Number of bits in message
 * @param method Method used to add ECC
 * 
 * @return Number of bits in ECC message
 */
uint16_t ErrorCorrection_CodedLength(const uint16_t length, 
                                     const ErrorCorrectionMethod_t method);

/**
 * @brief Returns number of bits in a segment after FEC is removed
 * 
 * @param length Length of the segment with FEC applied
 * @param method Method used for FEC
 * 
 * @return Number of raw message bits (no FEC)
 */
uint16_t ErrorCorrection_UncodedLength(const uint16_t length,
                                       const ErrorCorrectionMethod_t method);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_ERROR_CORRECTION_H_ */
