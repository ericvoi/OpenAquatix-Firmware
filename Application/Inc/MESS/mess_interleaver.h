/*
 * mess_interleaver.h
 *
 *  Created on: Jun 1, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_INTERLEAVER_H_
#define MESS_MESS_INTERLEAVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Interleaver_Apply(BitMessage_t* bit_msg, const DspConfig_t* cfg);
bool Interleaver_Undo(BitMessage_t* bit_msg, const DspConfig_t* cfg, bool is_preamble);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_INTERLEAVER_H_ */
