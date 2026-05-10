/*
 * mess_chirp.h
 *
 * Hard-coded LFM chirp TX path for the OpenCREST paper-experiments
 * channel-validation probe (§4.2). Emits a single 25→35 kHz, 50 ms
 * linear chirp through the production DAC chain. All RX-side detection
 * is done off-board on the host, so this module is intentionally
 * minimal (no menu params, no on-modem detection) and parallel to
 * mess_modulate.c for easy removal post-paper.
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
#define CHIRP_NUM_STEPS         500u
#define CHIRP_STEP_DURATION_US  (CHIRP_DURATION_US / CHIRP_NUM_STEPS)
#define CHIRP_AMPLITUDE         (1.0f)

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
