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
#include "mess_filt_resources.h"
#include "mess_demodulate.h"
#include "mess_background_noise.h"
#include "cfg_main.h"
#include "filt_main.h"
#include "sys_error.h"
#include "dac_waveform.h"
#include "goertzel.h"
#include "cfg_defaults.h"
#include <string.h>
#include <math.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  TRACKER_IDLE,
  TRACKER_SEARCHING
} PnTrackerState_t;

typedef struct {
  uint32_t start_cyccnt;
  uint16_t start_buffer_index;
  uint8_t symbols_exceeding_threshold;
  uint8_t frequencies_added;

  float capped_snr;
  float uncapped_snr;
  float ambient_snr;

  float doppler_alpha_sum;
  float doppler_alpha_weight;
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
#define PN_SYNC_SUBDIVIDE               16
#define MIN_NUM_CANDIDATES              (NUM_SYNC_FREQUENCIES * PN_SYNC_SUBDIVIDE)
#define NUM_PN_SYNC_CANDIDATES          (MIN_NUM_CANDIDATES * 4 / 3)

#define TARGET_SNR                      (8.0f)
#define CAP_RATIO                       (2.0f)
#define AMBIENT_RATIO                   (2.0f)
#define MIN_SYMBOLS_ABOVE_THRESH        24
#define REQUIRED_CANDIDATES_AFTER_BEST  8

// Frequency bank: all unique FHBFSK tones
#define MAX_FHBFSK_BANK_SIZE            (MAX_FHBFSK_NUM_TONES)

// Goertzel reset staggering
#define RESET_BLOCK_SIZE                8

// Doppler estimation via phase tracking
#define DOPPLER_PHASE_LOOKBACK          8
#define DOPPLER_ENERGY_THRESHOLD        3.0f

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

static volatile bool sync_error = false;

static float background_noise;

static bool pending_reset = true;

/*--- Frequency bank variables ---*/
static SlidingGoertzelInfo_t sliding_goertzel_bank[MAX_FHBFSK_BANK_SIZE];
static uint32_t bank_frequencies[MAX_FHBFSK_BANK_SIZE];
static uint16_t bank_size;
static int8_t sync_to_bank_map[NUM_SYNC_FREQUENCIES];

/*--- Doppler phase tracking variables ---*/
static float doppler_phase_buf_real[MAX_FHBFSK_BANK_SIZE][DOPPLER_PHASE_LOOKBACK];
static float doppler_phase_buf_imag[MAX_FHBFSK_BANK_SIZE][DOPPLER_PHASE_LOOKBACK];
static uint16_t doppler_sample_buf[DOPPLER_PHASE_LOOKBACK];
static uint16_t doppler_total_lookback_samples;
static uint8_t doppler_phase_buf_idx;
static uint8_t doppler_since_reset[MAX_FHBFSK_BANK_SIZE];

/* Private function prototypes -----------------------------------------------*/

static void updateParameters(const DspConfig_t* cfg);
static bool janusPnStep(bool* bit, uint16_t step);
static void fillJanusFrequencies(const DspConfig_t* cfg);
static void buildFrequencyBank(const DspConfig_t* cfg);
static SyncState_t janusPnSynchronize(Message_t* msg);
static bool resetPnSynchronization(void);
static void fillWindowOffsets(const DspConfig_t* cfg);
static bool updateSlidingGoertzel(uint16_t new_samples);
static SyncState_t evaluateSlidingPnWindows(Message_t* msg);
static void finalizeDopplerEstimate(Message_t* msg, uint16_t best_idx);
static float computeFineSyncOffset(uint16_t best_idx);

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
      return 0;
  }
}

SyncState_t Sync_Synchronize(const DspConfig_t* cfg, Message_t* msg)
{
  switch (cfg->sync_method) {
    case NO_SYNC: {
      bool ret = Input_DetectMessageStart(cfg, msg);
      return (ret == true) ? (SYNC_SUCCESS) : (SYNC_OK);
    }
    case SYNC_PN_32_JANUS:
      updateParameters(cfg);
      return janusPnSynchronize(msg);
    default:
      return SYNC_ERROR;
  }
}

void Sync_Reset(void)
{
  pending_reset = true;
}

/* Private function definitions ----------------------------------------------*/

static void updateParameters(const DspConfig_t* cfg)
{
  static uint32_t previous_version_number = 0;
  uint32_t current_version_number = CFG_GetVersionNumber();

  if (current_version_number == previous_version_number) {
    return;
  }
  previous_version_number = current_version_number;
  fillJanusFrequencies(cfg);
  buildFrequencyBank(cfg);
  fillWindowOffsets(cfg);
  resetPnSynchronization();
}

static bool janusPnStep(bool* bit, uint16_t step)
{
  if (step >= 32) return false;
  *bit = (janus_pn_32 >> (31 - step)) & 1;
  return true;
}

static void fillJanusFrequencies(const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    bool bit;
    janusPnStep(&bit, i);
    janus_frequencies[i] = Modulate_GetFhbfskFrequency(bit, i, cfg);
  }
}

static void buildFrequencyBank(const DspConfig_t* cfg)
{
  /* Collect all unique FHBFSK frequencies (both bit values, all symbol indices) */
  uint32_t all_freqs[NUM_SYNC_FREQUENCIES * 2];
  uint16_t count = 0;

  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    uint32_t f0 = Modulate_GetFhbfskFrequency(0, i, cfg);
    uint32_t f1 = Modulate_GetFhbfskFrequency(1, i, cfg);

    bool found0 = false, found1 = false;
    for (uint16_t j = 0; j < count; j++) {
      if (all_freqs[j] == f0) found0 = true;
      if (all_freqs[j] == f1) found1 = true;
    }
    if (!found0 && count < NUM_SYNC_FREQUENCIES * 2) all_freqs[count++] = f0;
    if (!found1 && count < NUM_SYNC_FREQUENCIES * 2) all_freqs[count++] = f1;
  }

  /* Insertion sort ascending */
  for (uint16_t i = 1; i < count; i++) {
    uint32_t key = all_freqs[i];
    int16_t j = (int16_t)i - 1;
    while (j >= 0 && all_freqs[j] > key) {
      all_freqs[j + 1] = all_freqs[j];
      j--;
    }
    all_freqs[j + 1] = key;
  }

  /* Detect minimum tone spacing */
  uint32_t min_spacing = UINT32_MAX;
  for (uint16_t i = 1; i < count; i++) {
    uint32_t diff = all_freqs[i] - all_freqs[i - 1];
    if (diff > 0 && diff < min_spacing) {
      min_spacing = diff;
    }
  }
  if (min_spacing == UINT32_MAX) {
    min_spacing = (uint32_t) roundf(cfg->baud_rate);
  }

  /* Build bank: lower edge + core tones + upper edge */
  bank_size = 0;

  if (all_freqs[0] > min_spacing && bank_size < MAX_FHBFSK_BANK_SIZE) {
    bank_frequencies[bank_size++] = all_freqs[0] - min_spacing;
  }

  for (uint16_t i = 0; i < count && bank_size < MAX_FHBFSK_BANK_SIZE; i++) {
    bank_frequencies[bank_size++] = all_freqs[i];
  }

  if (bank_size < MAX_FHBFSK_BANK_SIZE) {
    bank_frequencies[bank_size++] = all_freqs[count - 1] + min_spacing;
  }

  /* Map each sync symbol to its bank index */
  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    bool bit;
    janusPnStep(&bit, i);
    uint32_t f_active = Modulate_GetFhbfskFrequency(bit, i, cfg);

    sync_to_bank_map[i] = -1;

    for (uint16_t j = 0; j < bank_size; j++) {
      if (bank_frequencies[j] == f_active) sync_to_bank_map[i] = (int8_t)j;
    }
  }
}

static void fillWindowOffsets(const DspConfig_t* cfg)
{
  static const uint8_t offsets_precision = 4;
  samples_per_symbol = (uint16_t)((float) FILT_GetSamplingRate() / cfg->baud_rate);
  uint32_t offsets_increment = (samples_per_symbol << offsets_precision) / PN_SYNC_SUBDIVIDE;
  for (uint8_t i = 0; i < PN_SYNC_SUBDIVIDE; i++) {
    window_offsets[i] = (i * offsets_increment) >> offsets_precision;
  }
}

static bool resetPnSynchronization(void)
{
  offset_index = 0;
  candidate_index = 0;
  candidate_tracker.state = TRACKER_IDLE;
  candidate_tracker.num_candidates = 0;

  while (MessFiltResources_AvailableProcessingSamples() < samples_per_symbol) {
    osDelay(1);
  }

  /* Compute grouped-reset countdowns.
   * All filters within the same block share one countdown value.
   * Blocks are staggered so resets don't all land on the same sub-step. */
  uint16_t num_groups = (bank_size + RESET_BLOCK_SIZE - 1) / RESET_BLOCK_SIZE;

  for (uint16_t b = 0; b < bank_size; b++) {
    uint16_t group = b / RESET_BLOCK_SIZE;
    uint16_t countdown = (num_groups > 1)
        ? (group * SLIDING_GOERTZEL_CALLS_BEFORE_RESET / num_groups)
        : 0;

    goertzel_SlidingInit(&sliding_goertzel_bank[b], bank_frequencies[b],
                         samples_per_symbol, countdown);
    goertzel_SlidingReset(&sliding_goertzel_bank[b],
                          MessFiltResources_GetProcessingTail(), PROCESSING_BUFFER_SIZE);
  }

  /* Clear Doppler phase tracking state */
  memset(doppler_phase_buf_real, 0, sizeof(doppler_phase_buf_real));
  memset(doppler_phase_buf_imag, 0, sizeof(doppler_phase_buf_imag));
  memset(doppler_sample_buf, 0, sizeof(doppler_sample_buf));
  memset(doppler_since_reset, 0, sizeof(doppler_since_reset));
  doppler_total_lookback_samples = 0;
  doppler_phase_buf_idx = 0;

  memset(&pn_sync_candidates, 0, sizeof(pn_sync_candidates));
  MessFiltResources_ProcessingTailAdvance(samples_per_symbol);
  return true;
}

static SyncState_t janusPnSynchronize(Message_t* msg)
{
  if (pending_reset == true) {
    pending_reset = false;
    resetPnSynchronization();
  }

  bool bg_noise_ready = BackgroundNoise_Ready();
  background_noise = BackgroundNoise_GetScaleless();
  if (bg_noise_ready == false || background_noise == 0) {
    Sync_Reset();
    return SYNC_OK;
  }

  uint16_t next_idx = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
  uint16_t required_samples;
  if (next_idx == 0) {
    required_samples = samples_per_symbol - window_offsets[offset_index];
  } else {
    required_samples = window_offsets[next_idx] - window_offsets[offset_index];
  }

  while (MessFiltResources_AvailableProcessingSamples() >= required_samples) {
    if (updateSlidingGoertzel(required_samples) == false) {
      return SYNC_ERROR;
    }
    next_idx = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
    if (next_idx == 0) {
      required_samples = samples_per_symbol - window_offsets[offset_index];
    } else {
      required_samples = window_offsets[next_idx] - window_offsets[offset_index];
    }
  }

  return evaluateSlidingPnWindows(msg);
}

static bool updateSlidingGoertzel(uint16_t new_samples)
{
  /* --- Phase 1: Record pre-update state, detect resets --- */
  for (uint16_t b = 0; b < bank_size; b++) {
    if (sliding_goertzel_bank[b].calls_before_reset == 0) {
      doppler_since_reset[b] = 0;
    }
  }

  /* --- Phase 2: Update all bank filters with new samples --- */
  for (uint16_t b = 0; b < bank_size; b++) {
    goertzel_SlidingPerform(&sliding_goertzel_bank[b],
                            MessFiltResources_GetProcessingTail(), new_samples,
                            PROCESSING_BUFFER_SIZE);
  }

  /* --- Phase 3: Update lookback sample tracking for Doppler --- */
  doppler_total_lookback_samples -= doppler_sample_buf[doppler_phase_buf_idx];
  doppler_total_lookback_samples += new_samples;

  /* --- Phase 4: Initialize this candidate slot --- */
  uint64_t absolute_starting_index = MessFiltResources_TailRolloverCount(false)
                                     << PROCESSING_BUFFER_POWER;
  absolute_starting_index += MessFiltResources_GetProcessingTail() + new_samples;
  absolute_starting_index -= samples_per_symbol;
  pn_sync_candidates[candidate_index].start_buffer_index =
      absolute_starting_index & PROCESSING_BUFFER_MASK;
  pn_sync_candidates[candidate_index].frequencies_added = 0;
  pn_sync_candidates[candidate_index].capped_snr = 0;
  pn_sync_candidates[candidate_index].uncapped_snr = 0;
  pn_sync_candidates[candidate_index].symbols_exceeding_threshold = 0;
  pn_sync_candidates[candidate_index].doppler_alpha_sum = 0.0f;
  pn_sync_candidates[candidate_index].doppler_alpha_weight = 0.0f;
  pn_sync_candidates[candidate_index].start_cyccnt = MessFiltResources_AssociatedCyccnt(
      pn_sync_candidates[candidate_index].start_buffer_index, 
      absolute_starting_index >> PROCESSING_BUFFER_POWER);

  /* --- Phase 5: Evaluate each sync symbol's contribution to its candidate --- */
  float energy_threshold = DOPPLER_ENERGY_THRESHOLD * background_noise;

  for (uint16_t i = 0; i < NUM_SYNC_FREQUENCIES; i++) {
    int8_t k = sync_to_bank_map[i];

    /* Noise reference: next symbol's active frequency, or background for last */
    int8_t k_noise = (i < NUM_SYNC_FREQUENCIES - 1) ? sync_to_bank_map[i + 1] : -1;

    /* Fallback to background noise if mapping failed */
    float signal_e = (k >= 0) ? sliding_goertzel_bank[k].e_f : 0.0f;
    float noise_e;
    if (k_noise >= 0) {
      noise_e = MAX(sliding_goertzel_bank[k_noise].e_f, background_noise);
    } else {
      noise_e = background_noise;
    }

    float in_band_energy = MAX(signal_e - noise_e, 0.0f);
    float snr = in_band_energy / background_noise;
    float scaled_snr = snr / NUM_SYNC_FREQUENCIES;
    float capped_scaled_snr = MIN(scaled_snr,
                                  TARGET_SNR * CAP_RATIO / NUM_SYNC_FREQUENCIES);

    int16_t candidate_offset = (int16_t)candidate_index - (int16_t)(i * PN_SYNC_SUBDIVIDE);
    uint16_t sliding_index;
    if (candidate_offset < 0) {
      sliding_index = (uint16_t)(NUM_PN_SYNC_CANDIDATES + candidate_offset);
    } else {
      sliding_index = (uint16_t)candidate_offset;
    }

    pn_sync_candidates[sliding_index].capped_snr += capped_scaled_snr;
    pn_sync_candidates[sliding_index].uncapped_snr += scaled_snr;
    pn_sync_candidates[sliding_index].symbols_exceeding_threshold +=
        (snr > TARGET_SNR) ? 1 : 0;
    pn_sync_candidates[sliding_index].frequencies_added++;

    if (pn_sync_candidates[sliding_index].frequencies_added == NUM_SYNC_FREQUENCIES) {
      float sum = 0;
      for (uint16_t j = 0; j < bank_size; j++) {
        sum += sliding_goertzel_bank[j].e_f;
      }
      float average_energy = sum / (float)bank_size;
      pn_sync_candidates[sliding_index].ambient_snr =
          (average_energy - background_noise) / background_noise;
    }
    if (pn_sync_candidates[sliding_index].frequencies_added > NUM_SYNC_FREQUENCIES) {
      return false;
    }

    /* --- Per-symbol Doppler estimation via phase tracking --- */
    if (k >= 0 &&
        signal_e >= energy_threshold &&
        doppler_since_reset[k] >= DOPPLER_PHASE_LOOKBACK &&
        doppler_total_lookback_samples > 0) {

      /* Retrieve lookback complex value (oldest in circular buffer) */
      float old_real = doppler_phase_buf_real[k][doppler_phase_buf_idx];
      float old_imag = doppler_phase_buf_imag[k][doppler_phase_buf_idx];

      /* Check that lookback energy was also sufficient */
      float old_mag_sq = old_real * old_real + old_imag * old_imag;
      float threshold_unnorm = energy_threshold
                               / sliding_goertzel_bank[k].normalization_factor;

      if (old_mag_sq >= threshold_unnorm) {
        float new_real = sliding_goertzel_bank[k].x_real;
        float new_imag = sliding_goertzel_bank[k].x_imag;

        /* Z = X_new × conj(X_old): total phase rotation */
        float z_real = new_real * old_real + new_imag * old_imag;
        float z_imag = new_imag * old_real - new_real * old_imag;

        /* Expected rotation for nominal frequency over lookback span */
        float omega = sliding_goertzel_bank[k].omega;
        float total_samples_f = (float)doppler_total_lookback_samples;
        float expected_phase = omega * total_samples_f;
        float exp_r = cosf(expected_phase);
        float exp_i = sinf(expected_phase);

        /* Excess rotation = Z × conj(expected) */
        float exc_real = z_real * exp_r + z_imag * exp_i;
        float exc_imag = z_imag * exp_r - z_real * exp_i;

        if (exc_real > 0.0f) {
          float excess_phase = atan2f(exc_imag, exc_real);

          /* alpha = excess_phase / (omega × N)
           * Doppler causes f_rx = f_tx·(1+alpha) */
          float denom = omega * total_samples_f;
          if (fabsf(denom) > 1e-6f) {
            float alpha  = excess_phase / denom;
            float weight = signal_e;

            pn_sync_candidates[sliding_index].doppler_alpha_sum    += weight * alpha;
            pn_sync_candidates[sliding_index].doppler_alpha_weight += weight;
          }
        }
      }
    }
  }

  /* --- Phase 6: Increment since-reset counters --- */
  for (uint16_t b = 0; b < bank_size; b++) {
    if (doppler_since_reset[b] < UINT8_MAX) {
      doppler_since_reset[b]++;
    }
  }

  /* --- Phase 7: Save current complex values into phase circular buffer --- */
  for (uint16_t b = 0; b < bank_size; b++) {
    doppler_phase_buf_real[b][doppler_phase_buf_idx] = sliding_goertzel_bank[b].x_real;
    doppler_phase_buf_imag[b][doppler_phase_buf_idx] = sliding_goertzel_bank[b].x_imag;
  }
  doppler_sample_buf[doppler_phase_buf_idx] = new_samples;
  doppler_phase_buf_idx = (doppler_phase_buf_idx + 1) % DOPPLER_PHASE_LOOKBACK;

  offset_index = (offset_index + 1) % PN_SYNC_SUBDIVIDE;
  candidate_index = (candidate_index + 1) % NUM_PN_SYNC_CANDIDATES;
  candidate_tracker.num_candidates++;
  MessFiltResources_ProcessingTailAdvance(new_samples);
  return true;
}

/**
 * Finalize Doppler estimate from the best candidate's accumulated
 * per-symbol phase measurements.
 */
static void finalizeDopplerEstimate(Message_t* msg, uint16_t best_idx)
{
  PnSynchronizationCandidate_t* best = &pn_sync_candidates[best_idx];

  if (best->doppler_alpha_weight == 0.0f) {
    msg->doppler_mps = 0.0f;
    return;
  }

  float alpha = best->doppler_alpha_sum / best->doppler_alpha_weight;
  msg->doppler_mps = alpha * SPEED_OF_SOUND_MPS;
}

/**
 * Parabolic interpolation on the SNR peak for sub-subdivision timing.
 *
 * Returns a signed offset in samples.  Clamped to non-positive to prevent
 * multipath from biasing synchronization toward a later (reflected) arrival.
 * The direct path always arrives first, so we should never shift later.
 */
static float computeFineSyncOffset(uint16_t best_idx)
{
  uint16_t left_idx  = (best_idx == 0)
                        ? (NUM_PN_SYNC_CANDIDATES - 1)
                        : (best_idx - 1);
  uint16_t right_idx = (best_idx + 1) % NUM_PN_SYNC_CANDIDATES;

  /* Neighbours must be fully evaluated */
  if (pn_sync_candidates[left_idx].frequencies_added  != NUM_SYNC_FREQUENCIES ||
      pn_sync_candidates[right_idx].frequencies_added != NUM_SYNC_FREQUENCIES) {
    return 0.0f;
  }

  float y_left   = pn_sync_candidates[left_idx].uncapped_snr;
  float y_center = pn_sync_candidates[best_idx].uncapped_snr;
  float y_right  = pn_sync_candidates[right_idx].uncapped_snr;

  float denom = y_left - 2.0f * y_center + y_right;
  if (fabsf(denom) < 1e-6f) {
    return 0.0f;
  }

  /* delta is in units of sub-steps, range (-0.5, +0.5) */
  float delta = 0.5f * (y_left - y_right) / denom;

  float avg_substep_samples = (float)samples_per_symbol / (float)PN_SYNC_SUBDIVIDE;
  return delta * avg_substep_samples;
}

static SyncState_t evaluateSlidingPnWindows(Message_t* msg)
{
  while (candidate_tracker.num_candidates > (MIN_NUM_CANDIDATES + 1)) { // + 1 For fine synchronization
    int16_t s_idx = (int16_t)candidate_index
                    - (int16_t)candidate_tracker.num_candidates;
    uint16_t idx = (s_idx < 0)
                   ? (uint16_t)(NUM_PN_SYNC_CANDIDATES + s_idx)
                   : (uint16_t)s_idx;

    PnSynchronizationCandidate_t* candidate = &pn_sync_candidates[idx];
    if (candidate->frequencies_added != NUM_SYNC_FREQUENCIES) {
      return SYNC_ERROR;
    }

    bool exceeds_capped_snr   = candidate->capped_snr > TARGET_SNR;
    bool exceeds_symbol_count = candidate->symbols_exceeding_threshold
                                > MIN_SYMBOLS_ABOVE_THRESH;
    bool exceeds_target_snr   = candidate->uncapped_snr > TARGET_SNR;
    bool exceeds_ambient      = candidate->uncapped_snr
                                > (candidate->ambient_snr * AMBIENT_RATIO);

    switch (candidate_tracker.state) {
      case TRACKER_IDLE:
        if (exceeds_target_snr && exceeds_symbol_count &&
            exceeds_capped_snr && exceeds_ambient) {
          candidate_tracker.state = TRACKER_SEARCHING;
          candidate_tracker.best_index = idx;
          candidate_tracker.best_snr = candidate->uncapped_snr;
          candidate_tracker.candidates_after_best = 0;
        }
        break;

      case TRACKER_SEARCHING:
        if (candidate->uncapped_snr > candidate_tracker.best_snr &&
            exceeds_symbol_count) {
          candidate_tracker.best_index = idx;
          candidate_tracker.best_snr = candidate->uncapped_snr;
          candidate_tracker.candidates_after_best = 0;
        } 
        else {
          candidate_tracker.candidates_after_best++;
          if (candidate_tracker.candidates_after_best >= REQUIRED_CANDIDATES_AFTER_BEST) {
            /* --- Finalize Doppler estimate --- */
            finalizeDopplerEstimate(msg, candidate_tracker.best_index);

            /* --- Fine synchronization offset --- */
            float fine_offset = computeFineSyncOffset(candidate_tracker.best_index);
            int16_t fine_offset_int = (int16_t)roundf(fine_offset);

            /* --- Compute new processing tail --- */
            uint32_t samples_since_start = NUM_SYNC_FREQUENCIES * samples_per_symbol;

            uint32_t base_tail =
                (pn_sync_candidates[candidate_tracker.best_index].start_buffer_index
                 + samples_since_start) % PROCESSING_BUFFER_SIZE;

            uint32_t new_tail =
                (base_tail + (uint32_t)(fine_offset_int + PROCESSING_BUFFER_SIZE))
                & PROCESSING_BUFFER_MASK;

            msg->snr = candidate_tracker.best_snr;
            msg->rx_cyccnt = 
                pn_sync_candidates[candidate_tracker.best_index].start_cyccnt +
                (uint32_t)((int64_t)fine_offset_int * SystemCoreClock / FILT_GetSamplingRate());
            MessFiltResources_SetProcessingTail(new_tail);
            return SYNC_SUCCESS;
          }
        }
        break;

      default:
        return SYNC_ERROR;
    }

    candidate_tracker.num_candidates--;
  }
  return SYNC_OK;
}