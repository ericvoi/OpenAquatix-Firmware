/*
 * tr_switch.h
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef AFE_INTERNAL_TR_SWITCH_H_
#define AFE_INTERNAL_TR_SWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  TR_OUTPUT_MODE,   // Connected to the output of the power amplifier/impedance matching network only
  TR_INPUT_MODE,    // Connected to the pre-amplifier only
  TR_NONE           // Not connected to the input or output (power saving)
} TrState_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Changes whether the transducer is conencted to input, output, or none
 * 
 * @param new_state What the transducer should be connected to
 */
void TR_Change(TrState_t new_state);

/**
 * @brief Returns what the transducer is connected to
 * 
 * @return TrState_t What the transducer is currently connected to
 */
TrState_t TR_CurrentState(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_INTERNAL_TR_SWITCH_H_ */
