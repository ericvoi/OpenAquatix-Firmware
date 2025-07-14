/*
 * msss_evaluate.h
 *
 *  Created on: Mar 16, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_EVALUATE_H_
#define MESS_MESS_EVALUATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_main.h"
#include "mess_packet.h"


/* Private includes ----------------------------------------------------------*/

#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds evaluation message cargo to a message
 * 
 * @param bit_msg Bit message to add the BER evaluation cargo to
 * @return true if successful, false otherwise
 */
bool Evaluate_AddCargo(BitMessage_t* bit_msg);

/**
 * @brief Calculates the coded (with FEC) BER
 * 
 * @param eval_info Storage for results (modified)
 * @param bit_msg Received message to evaluate
 * @return true if successful, false otherwise
 * 
 * @note The cargo must be coded
 */
bool Evaluate_CodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg);

/**
 * @brief Calculates the uncoded (no FEC) BER
 * 
 * @param eval_info Storage for results (modified)
 * @param bit_msg Uncoded received message to evaluate
 * @param cfg DSP configuration values (uses fec method)
 * @return true if successful, false otherwise
 * 
 * @note The cargo must be uncoded (fec still applied)
 */
bool Evaluate_UncodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg, const DspConfig_t* cfg);

/**
 * @brief Registers the parameters used for the evaluation module
 * 
 * @return true if registered successfully, false otherwise
 */
bool Evaluate_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_EVALUATE_H_ */
