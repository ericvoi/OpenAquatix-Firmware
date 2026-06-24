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

/* Exported constants --------------------------------------------------------*/

#define CHIRP_F_START_HZ        25000u
#define CHIRP_F_END_HZ          35000u
#define CHIRP_DURATION_US       50000u
#define CHIRP_AMPLITUDE         (0.5f)

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Registers chirp parameters with the DAC resource layer and starts
 *        DAC output. Must be called from the MESS task context after
 *        switching the AFE/HIL state to TX (see enterDrivingTransducer()
 *        in mess_main.c). Physical routing is the caller's responsibility.
 *
 * The function bypasses the message-framing pipeline by reusing the test-
 * tone resource path (Waveform_SetWaveformSequence with is_message=false)
 * and walks N constant-frequency stair steps that the waveform engine
 * stitches into a phase-continuous LFM sweep.
 */
void MessChirp_StartTx(void);

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_CHIRP_H_ */
