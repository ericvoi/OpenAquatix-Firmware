/*
 * mess_sync.c
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_sync.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "mess_input.h"
#include "mess_modulate.h"
#include "mess_adc.h"
#include "mess_demodulate.h"
#include "mess_background_noise.h"
#include "cfg_main.h"
#include "dac_waveform.h"
#include "error_manager.h"
#include "goertzel.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  TRACKER_IDLE,
  TRACKER_SEARCHING
} PnTrackerState_t;

typedef struct {
  uint16_t start_buffer_index;

  float capped_snr;
  float uncapped_snr;
  float ambient_snr;
  uint8_t symbols_exceeding_threshold;

  uint8_t frequencies_added;
} PnSynchronizationCandidate_t;

typedef struct {
  PnTrackerState_t state;

  uint16_t num_candidates;
  uint16_t best_index;
  uint16_t candidates_after_best; 
  float best_snr;
} PnTracker_t;

/* Private define ------------------------------------------------------------*/

#define NUM_SYNC_FREQUENCIES            32
#define PN_SYNC_SUBDIVIDE               16 // The number of subdivisions to make of each chip duration during synchronization
#define MIN_NUM_CANDIDATES              (NUM_SYNC_FREQUENCIES * PN_SYNC_SUBDIVIDE)
#define NUM_PN_SYNC_CANDIDATES          (MIN_NUM_CANDIDATES * 4 / 3) // Extra 33% to prevent overrun

#define TARGET_SNR                      (8.0f)
#define CAP_RATIO                       (2.0f) // Capped SNR statistics are truncated to CAP_RATIO * TARGET_SNR
#define AMBIENT_RATIO                   (2.0f)
#define MIN_SYMBOLS_ABOVE_THRESH        24
#define REQUIRED_CANDIDATES_AFTER_BEST  8

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

static const uint32_t janus_pn_32 = 0b10101110110001111100110100100000U;
static uint32_t janus_frequencies[NUM_SYNC_FREQUENCIES];

static PnSynchronizationCandidate_t pn_sync_candidates[NUM_PN_SYNC_CANDIDATES];

static uint16_t samples_per_symbol;
static uint16_t offset_index;
static uint16_t candidate_index;
static uint16_t window_offsets[PN_SYNC_SUBDIVIDE];
static PnTracker_t candidate_tracker;

static SlidingGoertzelInfo_t sliding_goertzel[NUM_SYNC_FREQUENCIES];

static volatile bool sync_error = false;

static float background_noise;
static float most_recent_snr = 0.0f;

static bool pending_reset = true;

/* Private function prototypes -----------------------------------------------*/

static void updateParameters(const DspConfig_t* cfg);
static void janusPnStep(bool* bit, uint16_t step);
static void fillJanusFrequencies(const DspConfig_t* cfg);
static SyncState_t janusPnSynchronize();
static void resetPnSynchronization();
static void fillWindowOffsets(const DspConfig_t* cfg);
static void updateSlidingGoertzel(uint16_t new_samples);
static SyncState_t evaluateSlidingPnWindows();

/* Exported function definitions ---------------------------------------------*/

bool Sync_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step)
{
  switch (cfg->sync_method) {
    case NO_SYNC:
      return false;
    case SYNC_PN_32_JANUS:
      waveform_step->freq_hz = janus_frequencies[step];
      waveform_step->duration_us = (uint32_t) roundf(1000000.0f / cfg->baud_rate);
      waveform_step->relative_amplitude = Modulate_GetAmplitude(waveform_step->freq_hz);
      return true;
    default:
      return false;
  }
  return true;
}

uint16_t Sync_NumSteps(const DspConfig_t* cfg)
{
  switch (cfg->sync_method) {
    case NO_SYNC:
      return 0;
    case SYNC_PN_32_JANUS:
      return 32;
    default:
      return false;
  }
}

SyncState_t Sync_Synchronize(const DspConfig_t* cfg)
{
  updateParameters(cfg);
  switch (cfg->sync_method) {
    case NO_SYNC:
      return Input_DetectMessageStart(cfg);
    case SYNC_PN_32_JANUS:
      return janusPnSynchronize(cfg);
    default:
      REGISTER_ERROR_INT(ERROR_UNHANDLED_CASE, SYNC_OK);
      return SYNC_OK;
  }
}

float Sync_MostRecentSnr()
{
  return most_recent_snr;
}

void Sync_Reset()
{
  pending_reset = true;
}

/* Private function definitions ----------------------------------------------*/

void updateParameters(const DspConfig_t* cfg)
{
  static uint32_t previous_version_number = 0; 
  uint32_t current_version_number = CFG_GetVersionNumber(); 

  if (current_version_number == previous_version_number) {
    return; // No update needed
  }
  previous_version_number = current_version_number;
  fillJanusFrequencies(cfg);
  fillWindowOffsets(cfg);
}

void janusPnStep(bool* bit, uint16_t step)
{
  if (step >= 32) REGISTER_ERROR(ERROR_INVALID_FUNCTION_PARAMETERS);
  *bit = (janus_pn_32 >> (31 - step)) & 1;
}

// TODO: change for FSK and consider implications of penalty function
void fillJanusFrequencies(const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    bool bit;
    janusPnStep(&bit, i);
    janus_frequencies[i] = Modulate_GetFhbfskFrequency(bit, i, cfg);
  }
}

SyncState_t janusPnSynchronize()
{
  // update the buffer with the new samples. Add to candidate solutions
  if (pending_reset == true) {
    pending_reset = false;
    resetPnSynchronization();
  }

  bool bg_noise_ready = BackgroundNoise_Ready();
  background_noise = BackgroundNoise_Get();
  if (bg_noise_ready == false || background_noise == 0) {
    Sync_Reset();
    return SYNC_OK;
  }

  uint16_t next_idx = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
  uint16_t required_samples;
  if (next_idx == 0) {
    // Wraparound: remaining samples in this symbol
    required_samples = samples_per_symbol - window_offsets[offset_index];
  } 
  else {
    required_samples = window_offsets[next_idx] - window_offsets[offset_index];
  }

  while (ADC_InputAvailableSamples() >= required_samples) {
    RETURN_IF_ERROR_PRESENT_INT(updateSlidingGoertzel(required_samples), SYNC_OK);
    uint16_t next_idx = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
    if (next_idx == 0) {
      // Wraparound: remaining samples in this symbol
      required_samples = samples_per_symbol - window_offsets[offset_index];
    } else {
      required_samples = window_offsets[next_idx] - window_offsets[offset_index];
    }
  }

  return evaluateSlidingPnWindows();
}

void resetPnSynchronization()
{
  offset_index = 0;
  candidate_index = 0;
  candidate_tracker.state = TRACKER_IDLE;
  candidate_tracker.num_candidates = 0;
  while (ADC_InputAvailableSamples() < samples_per_symbol) {
    osDelay(1);
  }

  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    goertzel_SlidingInit(&sliding_goertzel[i], janus_frequencies[i], samples_per_symbol);
    goertzel_SlidingReset(&sliding_goertzel[i], ADC_InputGetTail(), PROCESSING_BUFFER_SIZE);
  }

  memset(&pn_sync_candidates, 0, sizeof(pn_sync_candidates));
  ADC_InputTailAdvance(samples_per_symbol);
}

void fillWindowOffsets(const DspConfig_t* cfg)
{
  // Adding additional precision when calculating the offsets has an especially
  // high significance when the baud rate is high and/or the samples per symbol
  // is far from a multiple of stage 1 subdivide. Without the additional
  // precision, there can be up to the subdivide/2 missing samples.
  static const uint8_t offsets_precision = 4;
  samples_per_symbol = (uint16_t) ((float) ADC_SAMPLING_RATE / cfg->baud_rate);
  uint32_t offsets_increment = (samples_per_symbol << offsets_precision) / PN_SYNC_SUBDIVIDE;
  for (uint8_t i = 0; i < PN_SYNC_SUBDIVIDE; i++) {
    window_offsets[i] = (i * offsets_increment) >> offsets_precision;
  }
}

void updateSlidingGoertzel(uint16_t new_samples)
{
  // Initializes each candidate
  uint64_t absolute_starting_index = ADC_TailRolloverCount(false) << PROCESSING_BUFFER_POWER;
  absolute_starting_index += ADC_InputGetTail() + new_samples;
  absolute_starting_index -= samples_per_symbol;
  pn_sync_candidates[candidate_index].start_buffer_index = absolute_starting_index & PROCESSING_BUFFER_MASK;
  pn_sync_candidates[candidate_index].frequencies_added = 0;
  pn_sync_candidates[candidate_index].capped_snr = 0;
  pn_sync_candidates[candidate_index].uncapped_snr = 0;
  pn_sync_candidates[candidate_index].symbols_exceeding_threshold = 0;


  // Loops through and updates each sliding goertzel filter and updates candidate accordingly
  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    goertzel_SlidingPerform(&sliding_goertzel[i], ADC_InputGetTail(), new_samples, PROCESSING_BUFFER_SIZE);
    float next_bin_e = (i == NUM_SYNC_FREQUENCIES - 1) ? (background_noise) : (MAX(sliding_goertzel[i + 1].e_f, background_noise));
    float in_band_energy = MAX(sliding_goertzel[i].e_f - next_bin_e, 0.0f);
    float snr = (in_band_energy) / background_noise;
    float scaled_snr = snr / NUM_SYNC_FREQUENCIES;
    float capped_scaled_snr = MIN(scaled_snr, TARGET_SNR * CAP_RATIO / NUM_SYNC_FREQUENCIES);
    int16_t candidate_offset = candidate_index - i * PN_SYNC_SUBDIVIDE;
    uint16_t sliding_index;
    if (candidate_offset < 0) {
      sliding_index = NUM_PN_SYNC_CANDIDATES + candidate_offset;
    } 
    else {
      sliding_index = candidate_offset;
    }
    pn_sync_candidates[sliding_index].capped_snr += capped_scaled_snr;
    pn_sync_candidates[sliding_index].uncapped_snr += scaled_snr;
    pn_sync_candidates[sliding_index].symbols_exceeding_threshold += (snr > TARGET_SNR) ? 1 : 0;
    pn_sync_candidates[sliding_index].frequencies_added++;
    if (pn_sync_candidates[sliding_index].frequencies_added == NUM_SYNC_FREQUENCIES) {
      float sum = 0;
      for (uint16_t j = 0; j < NUM_SYNC_FREQUENCIES; j++) {
        sum += sliding_goertzel[j].e_f;
      }
      float average_energy = sum / NUM_SYNC_FREQUENCIES;
      pn_sync_candidates[sliding_index].ambient_snr = (average_energy - background_noise) / background_noise;
    }
    if (pn_sync_candidates[sliding_index].frequencies_added > NUM_SYNC_FREQUENCIES) 
      REGISTER_ERROR(ERROR_OVERFLOW_SYNC);
  }

  offset_index = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
  candidate_index = (candidate_index + 1) % NUM_PN_SYNC_CANDIDATES;
  candidate_tracker.num_candidates++;
  ADC_InputTailAdvance(new_samples);
}

SyncState_t evaluateSlidingPnWindows()
{
  while (candidate_tracker.num_candidates > MIN_NUM_CANDIDATES) {
    int16_t s_idx = candidate_index - candidate_tracker.num_candidates;
    uint16_t idx = (s_idx < 0) ? (NUM_PN_SYNC_CANDIDATES + s_idx) : ((uint16_t) s_idx);
    PnSynchronizationCandidate_t* candidate = &pn_sync_candidates[idx];
    if (candidate->frequencies_added != NUM_SYNC_FREQUENCIES)  {
      REGISTER_ERROR_INT(ERROR_OVERFLOW_SYNC, SYNC_OK);
      return SYNC_OK;
    }
    bool exceeds_capped_snr = candidate->capped_snr > TARGET_SNR;
    bool exceeds_symbol_count = candidate->symbols_exceeding_threshold > MIN_SYMBOLS_ABOVE_THRESH;
    bool exceeds_target_snr = candidate->uncapped_snr > TARGET_SNR;
    bool exceeds_ambient = candidate->uncapped_snr > (candidate->ambient_snr * AMBIENT_RATIO);
    switch (candidate_tracker.state) {
      case TRACKER_IDLE:
        if (exceeds_target_snr && exceeds_symbol_count && exceeds_capped_snr && exceeds_ambient) {
          candidate_tracker.state = TRACKER_SEARCHING;
          candidate_tracker.best_index = idx;
          candidate_tracker.best_snr = candidate->uncapped_snr;
          candidate_tracker.candidates_after_best = 0;
        }
        break;

      case TRACKER_SEARCHING:
        if (candidate->uncapped_snr > candidate_tracker.best_snr && (candidate->symbols_exceeding_threshold > MIN_SYMBOLS_ABOVE_THRESH)) {
          candidate_tracker.best_index = idx;
          candidate_tracker.best_snr = candidate->uncapped_snr;
          candidate_tracker.candidates_after_best = 0;
        } else {
          candidate_tracker.candidates_after_best++;
          if (candidate_tracker.candidates_after_best >= REQUIRED_CANDIDATES_AFTER_BEST) {
            uint32_t new_tail = (pn_sync_candidates[candidate_tracker.best_index].start_buffer_index + NUM_SYNC_FREQUENCIES * samples_per_symbol) % PROCESSING_BUFFER_SIZE;
            most_recent_snr = candidate_tracker.best_snr;
            ADC_InputSetTail(new_tail);
            return SYNC_SUCCESS;
          }
        }
        break;
      default:
        REGISTER_ERROR_INT(ERROR_UNHANDLED_CASE, SYNC_OK);
        return SYNC_OK;
    }

    candidate_tracker.num_candidates--;
  }
  return SYNC_OK;
}

