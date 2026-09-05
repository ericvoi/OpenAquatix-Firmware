/*
 * mess_tvir.c
 *
 *  Created on: Jul 19, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_tvir.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "dac_waveform.h"
#include "filt_main.h"
#include "mess_chirp.h"
#include "mess_filt_resources.h"
#include "error_manager.h"
#include "janus_utils.h"

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  ModulationOutputType_t type;
  float prf;
  uint32_t probe_duration_ns;
  // Below values cannot exceed chirp silence duration (not checked)
  uint16_t extra_pretime_ms;  // Time before the main TVIR arrival to start correlating
  uint16_t extra_posttime_ms; // Default correlation window goes from chirp start -> chirp start + chirp_duration
} TvirDescription_t;

/* Private define ------------------------------------------------------------*/

#define TVIR_N_E              3
#define TVIR_N_X              4

#define INITIAL_SILENCE_MS    500

#define TVIR_START_FREQ       25000
#define TVIR_END_FREQ         35000

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static const TvirDescription_t tvir_types[NUM_TVIR_TYPES] = {
  {
    .type = OUTPUT_HFM,
    .prf = 50.0f,
    .probe_duration_ns = 10E6,
    .extra_pretime_ms = 2,
    .extra_posttime_ms = 5
  },
  {
    .type = OUTPUT_HFM,
    .prf = 25.0f,
    .probe_duration_ns = 20E6,
    .extra_pretime_ms = 5,
    .extra_posttime_ms = 12
  },
  {
    .type = OUTPUT_HFM,
    .prf = 10.0f,
    .probe_duration_ns = 50E6,
    .extra_pretime_ms = 5,
    .extra_posttime_ms = 35
  },
  {
    .type = OUTPUT_HFM,
    .prf = 5.0f,
    .probe_duration_ns = 100E6,
    .extra_pretime_ms = 5,
    .extra_posttime_ms = 80
  }
};

static uint16_t num_tx_sounding_probes = DEFAULT_NUM_TX_SOUNDING_PROBES;
static TvirTypes_t tx_tvir_type = DEFAULT_TX_TVIR_TYPE;

DEFINE_DESC_TABLE(TVIR_TYPE_TABLE, tvir_type_descriptors);

/* Private function prototypes -----------------------------------------------*/

// TVIR uses the same number of length calculating bits as JANUS, but assigns
// 3 granularity bits instead of 2 and 4 length bits instead of 5 for greater
// range
static uint16_t lengthCodeToProbes(uint16_t code);
static uint16_t probesToLengthCode(uint16_t n_probes);

/* Exported function definitions ---------------------------------------------*/

void Tvir_Init(void);

void Tvir_CalculateCargoCode(Message_t* msg, BitMessage_t* bit_msg)
{
  uint16_t encoded_len = probesToLengthCode(num_tx_sounding_probes);
  uint16_t num_probes = lengthCodeToProbes(encoded_len);
  msg->preamble.cargo_length.value = encoded_len;
  msg->preamble.cargo_length.valid = true;
  bit_msg->n_probes = num_probes;
  bit_msg->cargo.raw_len = 0;
  bit_msg->cargo.ecc_len = 0;
}

void Tvir_CalculateNProbes(Message_t* msg, BitMessage_t* bit_msg)
{
  if (msg->preamble.cargo_length.valid != true)
    REGISTER_ERROR(ERROR_INVALID_CARGO_LENGTH);
  
  uint16_t length_code = msg->preamble.cargo_length.value;
  uint16_t n_probes = lengthCodeToProbes(length_code);

  bit_msg->n_probes = n_probes;
}

void Tvir_Start(TvirTypes_t type, uint16_t length_code, uint16_t start_index, 
                uint32_t start_rollover)
{
  // TODO: check for PC connection to send data

  float duration_s = tvir_types[type].probe_duration_ns / 1.0E9;
  uint32_t f_s = FILT_GetSamplingRate();
  Chirp_UpdateTemplate(duration_s, tvir_types[type].type, TVIR_START_FREQ,
    TVIR_END_FREQ, f_s);

  // The current index/rollover is just the end of the preamble and only this
  // module is aware of the initial silence duration, so it is added here to
  // remove a condition from the run loop
  uint64_t actual_start = 0;
  actual_start |= start_index;
  actual_start |= start_rollover << 14;
  actual_start += (INITIAL_SILENCE_MS - tvir_types->extra_pretime_ms) / 1000.0f * f_s;
  uint64_t current_position = 0;
  current_position |= MessFiltResources_GetProcessingTail();
  current_position |= MessFiltResources_TailRolloverCount(false);
  while (current_position < actual_start) {
    if (actual_start - current_position < MessFiltResources_AvailableProcessingSamples()) {
      MessFiltResources_ProcessingTailAdvance(actual_start - current_position);
      return;
    }
    else {
      MessFiltResources_ProcessingTailAdvance(MessFiltResources_AvailableProcessingSamples());
    }
    osDelay(1);
    current_position = 0;
    current_position |= MessFiltResources_GetProcessingTail();
    current_position |= MessFiltResources_TailRolloverCount(false);
  }
  // TODO: Check if there are any unhandled cases
}
void Tvir_Run(void);

void Tvir_GetStep(Symbol_t* symbol, uint16_t n_probes, TvirTypes_t type, uint16_t tvir_index)
{
  RETURN_IF_ERROR_PRESENT();

  if (tvir_index == 0) {
    symbol->duration_ns = INITIAL_SILENCE_MS * 1000 * 1000;
    symbol->output_type = OUTPUT_NCO;
    symbol->ramp_samples = 0;
    symbol->relative_amplitude = 0.0f;
    return;
  }

  if (tvir_index >= (2 * n_probes))
    REGISTER_ERROR(ERROR_INVALID_TVIR_CONFIG);

  symbol->u.chirp.f_start_hz = TVIR_START_FREQ;
  symbol->u.chirp.f_end_hz = TVIR_END_FREQ;

  if ((tvir_index % 2) == 0) { // Even -> silence
    symbol->output_type = OUTPUT_NCO; // Cheapest modulator for power draw
    symbol->ramp_samples = 0;
    symbol->relative_amplitude = 0.0f;
    uint32_t period_samples = 1.0E9 / tvir_types[type].prf;
    symbol->duration_ns = period_samples - tvir_types[type].probe_duration_ns;
  }
  else { // Odd -> Chirp
    symbol->output_type = tvir_types[type].type;
    symbol->ramp_samples = 0;
    symbol->relative_amplitude = Modulate_GetAmplitude((TVIR_START_FREQ + TVIR_END_FREQ) / 2);
    symbol->duration_ns = tvir_types[type].probe_duration_ns;
  }
}

void Tvir_RegisterParams(void)
{
  uint16_t min_u16 = MIN_NUM_TX_SOUNDING_PROBES;
  uint16_t max_u16 = lengthCodeToProbes(0x7F);
  if (Param_Register(PARAM_NUM_TX_SOUNDING_PROBES, "Number of TVIR probes",
                     PARAM_TYPE_UINT16, &num_tx_sounding_probes, sizeof(uint16_t),
                     &min_u16, &max_u16, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  uint8_t min_u8 = MIN_TX_TVIR_TYPE;
  uint8_t max_u8 = MAX_TX_TVIR_TYPE;
  if (Param_Register(PARAM_TX_TVIR_TYPE, "TVIR type", PARAM_TYPE_ENUM, 
                     &tx_tvir_type, sizeof(TvirTypes_t), &min_u8, &max_u8, NULL,
                     tvir_type_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
}

/* Private function definitions ----------------------------------------------*/

static uint16_t lengthCodeToProbes(uint16_t code)
{
  uint16_t n_probes = JanusUtil_DecodeLen(code, TVIR_N_E, TVIR_N_X) / 8;
  RETURN_IF_ERROR_PRESENT_NON_VOID(, 0);
  return n_probes;
}

static uint16_t probesToLengthCode(uint16_t n_probes)
{
  uint16_t code = JanusUtil_EncodeLen(n_probes * 8, TVIR_N_E, TVIR_N_X);
  RETURN_IF_ERROR_PRESENT_NON_VOID(, 0);
  return code;
}
