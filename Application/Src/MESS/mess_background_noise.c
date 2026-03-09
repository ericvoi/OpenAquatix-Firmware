/*
 * mess_background_noise.c
 *
 *  Created on: Jul 14, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_dsp_config.h"
#include "mess_background_noise.h"
#include "mess_filt_resources.h"
#include "mess_demodulate.h"
#include "mess_modulate.h"
#include "mac_channel_reports.h"
#include "cfg_main.h"
#include "filt_main.h"
#include "arm_math.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float accumulated_energy;
  uint16_t counts;
} EnergyAccumulator_t;

typedef enum {
  BG_NOISE_BOOT,
  BG_NOISE_RUNNING
} BackgroundNoiseState_t;

/* Private define ------------------------------------------------------------*/

#define NOISE_BUFFER_SIZE           128
#define MS_PER_ENTRY                100
#define NOISE_HISTORY_SIZE          (1 << 6) // 64 or 6.4s

#define NOISE_ESTIMATION_PERCENTILE 25      // The noise value percentile to use
#define NOISE_ESTIMATION_BIAS       (1.05f) // Acounts for bias in using 25th percentile
#define NOISE_ESTIMATION_MIN_COUNT  10      // First noise estimation after 1s
#define NOISE_ESTIMATION_REJECTION  (5.0f)  // If a block is greater than a valid noise * this, it is rejected

#define ADC_V_SCALE                 (ADC_VREF / 2.0f)


/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

extern arm_rfft_fast_instance_f32 fft_handle128;

static EnergyAccumulator_t current_block;
static float noise_history[NOISE_HISTORY_SIZE];

static BackgroundNoiseState_t estimator_state = BG_NOISE_BOOT;

static uint16_t counts_per_entry = ADC_SAMPLING_RATE * MS_PER_ENTRY / 1000 / NOISE_BUFFER_SIZE;

// Two-sided PSD. Not normalized by sampling rate for ease of computation
static volatile float in_band_noise = 0;
static uint16_t noise_history_index = 0;
static uint16_t accumulated_noise_entries = 0;

static uint16_t lower_noise_bin = 0;
static uint16_t num_noise_bins = 1;

static ChannelReportType_t channel_report_type = REPORT_NONE;
static uint16_t current_counts_in_report = 0;
static uint16_t total_counts_in_report = 0;
static ChannelReport_t channel_report;

osEventFlagsId_t channel_report_flag = NULL;
osMessageQueueId_t channel_report_queue = NULL;

/* Private function prototypes -----------------------------------------------*/

static void updateBackgroundNoise();
static bool updateFrequencyIndices(const DspConfig_t* cfg);

static float computePercentile(const float* buf, uint16_t count);

static bool fskFrequencyIndices(const DspConfig_t* cfg);
static bool fhbfskFrequencyIndices(const DspConfig_t* cfg);
static bool updateNoiseIndices(uint32_t f0, uint32_t f1);

static float frequencyToIndex(uint32_t frequency, uint16_t fft_size);

static bool updateChannelReport();
static bool channelReportingRequirement(const DspConfig_t* cfg);
static bool updateChannelReportTotalCount(const DspConfig_t* cfg);

/* Exported function definitions ---------------------------------------------*/

bool BackgroundNoise_Init()
{
  channel_report_flag = osEventFlagsNew(NULL);
  if (channel_report_flag == NULL) {
    return false;
  }
  osEventFlagsClear(channel_report_flag, 0xFFFFFFFF);

  channel_report_queue = osMessageQueueNew(CHANNEL_REPORT_QUEUE_SIZE, sizeof(ChannelReport_t), NULL);
  if (channel_report_queue == NULL) {
    return false;
  }

  BackgroundNoise_Reset();
  return true;
}

void BackgroundNoise_Reset()
{
  MessFiltResources_SetNoiseTail(0);
  noise_history_index = 0;
  in_band_noise = 0;
  accumulated_noise_entries = 0;
  current_block.accumulated_energy = 0.0f;
  current_block.counts = 0;
  estimator_state = BG_NOISE_BOOT;
}

bool BackgroundNoise_Calculate(const DspConfig_t* cfg)
{
  if (updateFrequencyIndices(cfg) == false) {
    return false;
  }

  if (channelReportingRequirement(cfg) == false) {
    return false;
  }
  while (MessFiltResources_AvailableNoiseSamples() > NOISE_BUFFER_SIZE) {
    float fft_in_buf[NOISE_BUFFER_SIZE];
    float fft_out_buf[NOISE_BUFFER_SIZE];

    for (uint16_t i = 0; i < NOISE_BUFFER_SIZE; i++) {
      fft_in_buf[i] = MessFiltResources_GetNoiseData(i);
    }
    arm_rfft_fast_f32(&fft_handle128, fft_in_buf, fft_out_buf, 0);

    for (uint16_t j = 0; j < num_noise_bins; j++) {
      uint16_t index = (lower_noise_bin + j) % (NOISE_BUFFER_SIZE / 2);
      float real = fft_out_buf[2 * index];
      float imag = fft_out_buf[2 * index + 1];

      // Normalize to PSD (P/Hz)
      float mag = (real * real + imag * imag) / NOISE_BUFFER_SIZE;
      
      float increment = mag / ((float) num_noise_bins);
      current_block.accumulated_energy += increment;
      channel_report.psd += increment;
    }
    current_block.counts++;
    updateBackgroundNoise();
    if (updateChannelReport() == false) {
      return false;
    }
    MessFiltResources_NoiseTailAdvance(NOISE_BUFFER_SIZE);
  }
  return true;
}

float BackgroundNoise_GetScaleless()
{
  return in_band_noise;
}

float BackgroundNoise_GetNsd()
{
  float psd_two_sided = in_band_noise / FILT_GetBandwidth() / 2.0f;
  float psd_one_sided = psd_two_sided * 2.0f * (ADC_V_SCALE * ADC_V_SCALE);
  return sqrtf(psd_one_sided) * 1.0e9f;
}

bool BackgroundNoise_Ready()
{
  return estimator_state == BG_NOISE_RUNNING;
}

/* Private function definitions ----------------------------------------------*/

void updateBackgroundNoise()
{
  if (current_block.counts < counts_per_entry) return;
  current_block.accumulated_energy /= current_block.counts;
  accumulated_noise_entries = MIN(NOISE_HISTORY_SIZE, accumulated_noise_entries + 1);

  switch (estimator_state) {
    case BG_NOISE_BOOT:
      noise_history[noise_history_index] = current_block.accumulated_energy;
      if (accumulated_noise_entries >= NOISE_ESTIMATION_MIN_COUNT) {
        in_band_noise = computePercentile(noise_history, accumulated_noise_entries)
                        * NOISE_ESTIMATION_BIAS;
        estimator_state = BG_NOISE_RUNNING;
      }
      noise_history_index = (noise_history_index + 1) % NOISE_HISTORY_SIZE; 
      break;
    case BG_NOISE_RUNNING:
      if (current_block.accumulated_energy > (in_band_noise * NOISE_ESTIMATION_REJECTION))
        break;
      noise_history[noise_history_index] = current_block.accumulated_energy;
      in_band_noise = computePercentile(noise_history, accumulated_noise_entries)
                      * NOISE_ESTIMATION_BIAS;
      noise_history_index = (noise_history_index + 1) % NOISE_HISTORY_SIZE; 
      break;
    default:
      break;
  }
  current_block.accumulated_energy = 0.0f;
  current_block.counts = 0;
}

bool updateFrequencyIndices(const DspConfig_t* cfg)
{
  static uint32_t previous_version_number = 0; 
  uint32_t current_version_number = CFG_GetVersionNumber(); 

  if (current_version_number == previous_version_number) {
    return true; // No updated needed
  }

  previous_version_number = current_version_number;

  counts_per_entry = FILT_GetBandwidth() * 2 * MS_PER_ENTRY / 1000 / NOISE_BUFFER_SIZE;

  switch (cfg->mod_demod_method) {
    case MOD_DEMOD_FSK:
      return fskFrequencyIndices(cfg);
    case MOD_DEMOD_FHBFSK:
      return fhbfskFrequencyIndices(cfg);
    default:
      return false;
  }
}

// Uses insertion sort then finds the appropriate percentile
float computePercentile(const float* buf, uint16_t count)
{
  float tmp[NOISE_HISTORY_SIZE];
  memcpy(tmp, buf, count * sizeof(float));

  for (uint16_t i = 1; i < count; i++) {
    float key = tmp[i];
    int16_t j = (int16_t)i - 1;
    while (j >= 0 && tmp[j] > key) {
      tmp[j + 1] = tmp[j];
      j--;
    }
    tmp[j + 1] = key;
  }

  uint16_t k = (uint16_t)((uint32_t)count * NOISE_ESTIMATION_PERCENTILE / 100u);
  if (k >= count) k = count - 1;
  return tmp[k];
}

bool fskFrequencyIndices(const DspConfig_t* cfg)
{
  uint32_t f0 = cfg->fsk_f0;
  uint32_t f1 = cfg->fsk_f1;
  return updateNoiseIndices(f0, f1);
}

bool fhbfskFrequencyIndices(const DspConfig_t* cfg)
{
  uint32_t f0 = Modulate_GetFhbfskFrequency(false, 0, cfg);
  uint32_t f1 = f0 + (uint16_t) (cfg->baud_rate * ((2 * cfg->fhbfsk_num_tones - 1) * cfg->fhbfsk_freq_spacing));
  return updateNoiseIndices(f0, f1);
}

bool updateNoiseIndices(uint32_t f0, uint32_t f1)
{
  uint16_t noise_bin0 = (uint16_t) roundf(frequencyToIndex(f0, NOISE_BUFFER_SIZE));
  uint16_t noise_bin1 = (uint16_t) roundf(frequencyToIndex(f1, NOISE_BUFFER_SIZE));

  if (noise_bin0 <= noise_bin1) {
    lower_noise_bin = noise_bin0;
    num_noise_bins = noise_bin1 - noise_bin0 + 1;
  } 
  else {
    lower_noise_bin = noise_bin1;
    num_noise_bins = noise_bin0 - noise_bin1 + 1;
  }

  return true;
}

float frequencyToIndex(uint32_t frequency, uint16_t fft_size)
{
  float folded_frequency = (float) FILT_PassbandToBaseband(frequency);
  return folded_frequency * fft_size / (FILT_GetBandwidth() * 2.0f);
}

bool updateChannelReport()
{
  switch (channel_report_type) {
    case REPORT_NONE:
      return true;
    case REPORT_16_CD_PSD:
      current_counts_in_report++;
      if (current_counts_in_report >= total_counts_in_report) {
        channel_report.psd /= current_counts_in_report;
        current_counts_in_report = 0;
        if (osMessageQueueGetSpace(channel_report_queue) == 0) {
          return false;
        }
        if (osMessageQueuePut(channel_report_queue, &channel_report, 0, 0) != osOK) {
          return false;
        }
        channel_report.psd = 0.0f;
      }
      return true;
    default:
      return false;
  }
}

bool channelReportingRequirement(const DspConfig_t* cfg)
{
  static ChannelReportType_t last_report_type = REPORT_NONE;
  static uint32_t last_cfg_number = 0;
  uint32_t current_cfg_number = CFG_GetVersionNumber();
  if (last_cfg_number != current_cfg_number) {
    last_cfg_number = current_cfg_number;
    switch (last_report_type) {
      case REPORT_NONE:
        break;
      case REPORT_16_CD_PSD:
        return updateChannelReportTotalCount(cfg);
      default:
        return false;
    }
  }

  uint32_t flags = osEventFlagsGet(channel_report_flag);
  if (flags == 0) {
    return true;
  }

  if (flags & REPORT_NONE) {
    channel_report_type = REPORT_NONE;
  }
  else if (flags & REPORT_16_CD_PSD) {
    channel_report_type = REPORT_16_CD_PSD;
  }

  if (channel_report_type == last_report_type) {
    return true;
  }

  switch (channel_report_type) {
    case REPORT_NONE:
      break;
    case REPORT_16_CD_PSD:
      return updateChannelReportTotalCount(cfg);
    default:
      return false;
  }
  return true;
}

bool updateChannelReportTotalCount(const DspConfig_t* cfg)
{
  if (cfg->mod_demod_method != MOD_DEMOD_FSK && cfg->mod_demod_method != MOD_DEMOD_FHBFSK) {
    return false;
  }
  uint32_t samples_per_chip = (uint32_t) (FILT_GetBandwidth() * 2.0f) / cfg->baud_rate;
  uint32_t samples_per_report = samples_per_chip * CHANNEL_REPORT_CD;
  uint32_t new_total_counts_in_report = (samples_per_report + NOISE_BUFFER_SIZE - 1) / NOISE_BUFFER_SIZE;

  if (total_counts_in_report != new_total_counts_in_report) {
    channel_report.psd = 0.0f;
    current_counts_in_report = 0;
    total_counts_in_report = new_total_counts_in_report;
  }
  return true;
}
