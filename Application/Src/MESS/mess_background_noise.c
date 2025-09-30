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
#include "mess_adc.h"
#include "mess_demodulate.h"
#include "mess_modulate.h"
#include "mac_channel_reports.h"
#include "cfg_main.h"
#include "arm_math.h"
#include "cmsis_os.h"
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float accumulated_energy;
  uint16_t counts;
} EnergyHistory_t;

/* Private define ------------------------------------------------------------*/

#define NOISE_BUFFER_SIZE       128
#define MS_PER_ENTRY            100
#define COUNTS_PER_ENTRY        (ADC_SAMPLING_RATE * MS_PER_ENTRY / 1000 / NOISE_BUFFER_SIZE)
#define NOISE_HISTORY_SIZE      (1 << 4) // 16

#define NUM_NOISE_IN_AVERAGE    (1 << 3) // 8

#define WINDOW_INCREMENT        4

#define NOISE_OUTLIER_THRESHOLD (1.1f)

#define RESET_NOISE_VALUE       (0.0f)

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

extern arm_rfft_fast_instance_f32 fft_handle128;

static bool energy_ready = false;

static EnergyHistory_t energy_history[NOISE_HISTORY_SIZE];

static volatile float in_band_noise = RESET_NOISE_VALUE;
static uint16_t noise_buffer_tail = 0;
static uint16_t noise_history_index = 0;
static uint16_t accumulated_noise_entries = 0;

static uint16_t lower_noise_bin = 0;
static uint16_t upper_noise_bin = 0;
static uint16_t num_noise_bins = 1;

static ChannelReportType_t channel_report_type = REPORT_NONE;
static uint16_t current_counts_in_report = 0;
static uint16_t total_counts_in_report = 0;
static ChannelReport_t channel_report;

osEventFlagsId_t channel_report_flag = NULL;
osMessageQueueId_t channel_report_queue = NULL;

/* Private function prototypes -----------------------------------------------*/

static void updateBackgroundNoise();
static void averageNoise();
static bool updateFrequencyIndices(const DspConfig_t* cfg);

static bool fskFrequencyIndices(const DspConfig_t* cfg);
static bool fhbfskFrequencyIndices(const DspConfig_t* cfg);

static float frequencyToIndex(float frequency, uint16_t fft_size);

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
  return true;
}

void BackgroundNoise_Reset()
{
  noise_buffer_tail = 0;
  accumulated_noise_entries = 0;
  noise_history_index = 0;
  in_band_noise = RESET_NOISE_VALUE;
  energy_ready = false;
}

bool BackgroundNoise_Calculate(const DspConfig_t* cfg)
{
  if (updateFrequencyIndices(cfg) == false) {
    return false;
  }

  if (channelReportingRequirement(cfg) == false) {
    return false;
  }

  uint16_t head = ADC_InputGetHead();
  while (((head - noise_buffer_tail) & PROCESSING_BUFFER_MASK) > NOISE_BUFFER_SIZE) {
    float fft_in_buf[NOISE_BUFFER_SIZE];
    float fft_out_buf[NOISE_BUFFER_SIZE];

    for (uint16_t i = 0; i < NOISE_BUFFER_SIZE; i++) {
      uint16_t index = (noise_buffer_tail + i) & PROCESSING_BUFFER_MASK;
      fft_in_buf[i] = ADC_InputGetDataAbsolute(index);
    }
    arm_rfft_fast_f32(&fft_handle128, fft_in_buf, fft_out_buf, 0);

    for (uint16_t j = lower_noise_bin; j < upper_noise_bin; j++) {
      float real = fft_out_buf[2 * j];
      float imag = fft_out_buf[2 * j + 1];

      // Normalize to PSD (P/Hz)
      float mag = (real * real + imag * imag) / NOISE_BUFFER_SIZE;
      
      float increment = mag / ((float) num_noise_bins);
      energy_history[noise_history_index].accumulated_energy += increment;
      channel_report.psd += increment;
    }
    energy_history[noise_history_index].counts++;
    updateBackgroundNoise();
    if (updateChannelReport() == false) {
      return false;
    }
    noise_buffer_tail = (noise_buffer_tail + NOISE_BUFFER_SIZE) & PROCESSING_BUFFER_MASK;
  }
  return true;
}

float BackgroundNoise_Get()
{
  return in_band_noise;
}

bool BackgroundNoise_Ready()
{
  return energy_ready;
}

/* Private function definitions ----------------------------------------------*/

void updateBackgroundNoise()
{
  if (energy_history[noise_history_index].counts >= COUNTS_PER_ENTRY) {
    energy_history[noise_history_index].accumulated_energy /= energy_history[noise_history_index].counts;
    // Check for very first noise entry or if in-line with previous values
    if (in_band_noise == RESET_NOISE_VALUE || 
      (energy_history[noise_history_index].accumulated_energy < (in_band_noise * (NOISE_OUTLIER_THRESHOLD)))) {
      accumulated_noise_entries = MIN(accumulated_noise_entries + 1, NUM_NOISE_IN_AVERAGE);
      noise_history_index = (noise_history_index + 1) % NOISE_HISTORY_SIZE;
      averageNoise();
    }
    energy_history[noise_history_index].counts = 0;
    energy_history[noise_history_index].accumulated_energy = 0.0f;
  }
}

void averageNoise()
{
  if (accumulated_noise_entries == 0) {
    return;
  }
  float average = 0.0f;
  for (uint16_t i = 0; i < accumulated_noise_entries; i++) {
    uint16_t offset_index = (noise_history_index - 1 - i) & (NOISE_HISTORY_SIZE - 1);
    float energy = energy_history[offset_index].accumulated_energy;
    average += energy;
  }
  average /= ((float) accumulated_noise_entries);
  in_band_noise = average;
  energy_ready = accumulated_noise_entries == NUM_NOISE_IN_AVERAGE;
}

bool updateFrequencyIndices(const DspConfig_t* cfg)
{
  static uint32_t previous_version_number = 0; 
  uint32_t current_version_number = CFG_GetVersionNumber(); 

  if (current_version_number == previous_version_number) {
    return true; // No updated needed
  }

  previous_version_number = current_version_number;

  switch (cfg->mod_demod_method) {
    case MOD_DEMOD_FSK:
      return fskFrequencyIndices(cfg);
    case MOD_DEMOD_FHBFSK:
      return fhbfskFrequencyIndices(cfg);
    default:
      return false;
  }
}

bool fskFrequencyIndices(const DspConfig_t* cfg)
{
  lower_noise_bin = (uint16_t) roundf(frequencyToIndex(cfg->fsk_f0, NOISE_BUFFER_SIZE));
  upper_noise_bin = (uint16_t) roundf(frequencyToIndex(cfg->fsk_f1, NOISE_BUFFER_SIZE));

  if (lower_noise_bin > upper_noise_bin) {
    uint16_t tmp = lower_noise_bin;
    lower_noise_bin = upper_noise_bin;
    upper_noise_bin = tmp;
  }

  if (upper_noise_bin > NOISE_BUFFER_SIZE / 2) {
    return false;
  }

  num_noise_bins = upper_noise_bin - lower_noise_bin + 1;
  return true;
}

bool fhbfskFrequencyIndices(const DspConfig_t* cfg)
{
  uint16_t lower_freq = Modulate_GetFhbfskFrequency(false, 0, cfg);
  uint16_t upper_freq = lower_freq + (uint16_t) (cfg->baud_rate * ((2 * cfg->fhbfsk_num_tones - 1) * cfg->fhbfsk_freq_spacing));

  lower_noise_bin = (uint16_t) roundf(frequencyToIndex(lower_freq, NOISE_BUFFER_SIZE));
  upper_noise_bin = (uint16_t) roundf(frequencyToIndex(upper_freq, NOISE_BUFFER_SIZE));

  if (upper_noise_bin > NOISE_BUFFER_SIZE / 2) {
    return false;
  }

  num_noise_bins = upper_noise_bin - lower_noise_bin + 1;
  return true;
}

float frequencyToIndex(float frequency, uint16_t fft_size)
{
  return frequency * fft_size / ((float) ADC_SAMPLING_RATE);
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
  uint32_t samples_per_chip = (uint32_t) ((float) ADC_SAMPLING_RATE) / cfg->baud_rate;
  uint32_t samples_per_report = samples_per_chip * CHANNEL_REPORT_CD;
  uint32_t new_total_counts_in_report = (samples_per_report + NOISE_BUFFER_SIZE - 1) / NOISE_BUFFER_SIZE;

  if (total_counts_in_report != new_total_counts_in_report) {
    channel_report.psd = 0.0f;
    current_counts_in_report = 0;
    total_counts_in_report = new_total_counts_in_report;
  }
  return true;
}
