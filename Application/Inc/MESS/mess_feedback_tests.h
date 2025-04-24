/*
 * mess_feedback_tests.h
 *
 *  Created on: Apr 21, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_FEEDBACK_TESTS_H_
#define MESS_MESS_FEEDBACK_TESTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_main.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Intiializes feedback network module
 * 
 * @return true if initialization successful, false otherwise
 * 
 * @note This must be called before using any FeedbackTests_... module
 */
bool FeedbackTests_Init(void);

/**
 * @brief Starts feedback tests
 * 
 * Sets flags in module such that the next time a module function is called,
 * it performs its action
 * 
 * @note Only call once as this function effectively resets the module
 */
void FeedbackTests_Start(void);

/**
 * @brief If performing a feedback test, sends the next message
 */
void FeedbackTests_GetNext(void);

/**
 * @brief Corrupts the bit message before it is sent out by flipping bits
 * 
 * If a feedback test is not ongoing, then this function returns withour doing
 * anything
 * 
 * @param bit_msg Message to add the bit errors to (modified)
 * 
 * @return true if successful even if no errors added
 */
bool FeedbackTests_CorruptMessage(BitMessage_t* bit_msg);
// returns true if doing a feedback network test and false otherwise
/**
 * @brief Compares the received message against the feedback network test
 * 
 * This function returns immediately if a feedback test is not ongoing
 * 
 * @param received_msg Received and decoded message to compare
 * @param received_bit_msg Received bits
 * @return true if performing a test and no errors, false otherwise
 */
bool FeedbackTests_Check(Message_t* received_msg, BitMessage_t* received_bit_msg);

/**
 * @brief returns the current configuration if currently running a feedback test
 *
 * @param cfg Current test's configuration
 *
 * @return true if doing a feedback test and false otherwise
 * @note If this returns false, then the configuration must be set to default
 */
bool FeedbackTests_GetConfig(const DspConfig_t** cfg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_FEEDBACK_TESTS_H_ */
