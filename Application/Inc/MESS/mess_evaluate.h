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


/* Private includes ----------------------------------------------------------*/

#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes evaluation message patterns
 *
 * Sets up five predefined test messages with different patterns:
 * - msg1-msg3: filled with constant byte values
 * - msg4: filled with bytes derived from 16-bit sequence
 * - msg5: filled with bytes derived from 32-bit sequence
 *
 * @return Always returns true
 */
bool Evaluate_Init(void);

/**
 * @brief Retrieves a specific bit from one of the evaluation messages
 *
 * @param message_index Index of the evaluation message to use
 * @param bit_index Bit position to retrieve (0 to EVAL_MESSAGE_LENGTH-1)
 * @param bit Output parameter to store the retrieved bit value
 *
 * @return true if bit was successfully retrieved, false on invalid parameters
 */
bool Evaluate_GetBit(uint8_t message_index, uint16_t bit_index, bool* bit);

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
bool Evaluate_CalculateBitErrorRate(EvalMessageInfo_t* data, Message_t* msg, uint8_t message_index);

/**
 * @brief Retrieves a specific bit from a message structure
 *
 * @param msg Pointer to message structure
 * @param bit_index Bit position to retrieve (0 to EVAL_MESSAGE_LENGTH-1)
 * @param bit Output parameter to store the retrieved bit value
 *
 * @return true if bit was successfully retrieved, false on invalid parameters
 */
bool Evaluate_GetMessageBit(Message_t* msg, uint16_t bit_index, bool* bit);

/**
 * @brief Copies a configured evaluation message into a message structure
 *
 * Retrieves the evaluation message index from PARAM_EVAL_MESSAGE,
 * then copies the corresponding evaluation message into the provided
 * message structure and sets its length.
 *
 * @param msg Destination message structure
 *
 * @return true if copying successful, false on parameter retrieval error
 */
bool Evaluate_CopyEvaluationMessage(Message_t* msg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_EVALUATE_H_ */
