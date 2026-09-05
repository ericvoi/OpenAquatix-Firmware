/*
 * mess_tvir.h
 *
 *  Created on: Jul 19, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_TVIR_H_
#define MESS_MESS_TVIR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "cfg_parameters.h"
#include "dac_waveform.h"
#include "mess_main.h"
#include "mess_packet.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define TVIR_TYPE_TABLE(X) \
  X(TVIR_50, "50Hz PRF") \
  X(TVIR_25, "25Hz PRF") \
  X(TVIR_10, "10Hz PRF") \
  X(TVIR_5, "5Hz PRF")

DECLARE_ENUM(TVIR_TYPE_TABLE, NUM_TVIR_TYPES, TvirTypes_t);

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void Tvir_Init(void);
void Tvir_CalculateCargoCode(Message_t* msg, BitMessage_t* bit_msg);
void Tvir_CalculateNProbes(Message_t* msg, BitMessage_t* bit_msg);
void Tvir_Start(TvirTypes_t type, uint16_t length_code, uint16_t start_index, 
                uint32_t start_rollover);
void Tvir_Run(void);
void Tvir_GetStep(Symbol_t* symbol, uint16_t n_probes, TvirTypes_t type, uint16_t tvir_index);
void Tvir_RegisterParams(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_TVIR_H_ */
