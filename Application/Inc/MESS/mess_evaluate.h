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

bool Evaluate_AddCargo(BitMessage_t* bit_msg);

/**
 * @brief Calculates bit error rate between a message and reference pattern
 *
 * Compares each bit in the provided message against a reference evaluation
 * message and calculates the proportion of differing bits.
 *
 * @param data Output structure where bit error rate will be stored
 * @param msg Message to evaluate against reference
 * @param message_index Index of the reference evaluation message
 *
 * @return true if calculation successful, false on error
 */
bool Evaluate_CodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg);

bool Evaluate_UncodedBer(EvalMessageInfo_t* eval_info, BitMessage_t* bit_msg, const DspConfig_t* cfg);

bool Evaluate_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_EVALUATE_H_ */
