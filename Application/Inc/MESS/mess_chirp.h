/*
 * mess_chirp.h
 *
 *  Created on: May 8, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_CHIRP_H_
#define MESS_MESS_CHIRP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>
#include "dac_waveform.h"

/* Exported constants --------------------------------------------------------*/

#define CHIRP_F_START_HZ        25000u
#define CHIRP_F_END_HZ          35000u
#define CHIRP_DURATION_US       50000u
#define CHIRP_AMPLITUDE         (0.5f)

/* Exported functions prototypes ---------------------------------------------*/

void Chirp_UpdateTemplate(float duration_s, ModulationOutputType_t type,
  uint32_t f_start, uint32_t f_end, uint32_t f_s);

/**
 * @brief Registers chirp parameters with the DAC resource layer and starts
 *        DAC output. Must be called from the MESS task context after
 *        switching the AFE/HIL state to TX (see enterDrivingTransducer()
 *        in mess_main.c). Physical routing is the caller's responsibility.
 */
void Chirp_TestChirp(void);



#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_CHIRP_H_ */
