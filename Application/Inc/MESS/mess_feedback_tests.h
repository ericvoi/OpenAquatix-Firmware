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
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool FeedbackTests_Init(void);
void FeedbackTests_Start(void);
void FeedbackTests_GetNext(void);
bool FeedbackTests_CorruptMessage(BitMessage_t* bit_msg);
// returns true if doing a feedback network test and false otherwise
bool FeedbackTests_Check(Message_t* received_msg, BitMessage_t* received_bit_msg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_FEEDBACK_TESTS_H_ */
