/*
 * mess_sync.c
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
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
#include "goertzel.h"
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  PN_STAGE_1,
  PN_STAGE_2,
  PN_STAGE_3,
  PN_STAGE_4,
  PN_STAGE_COMPLETE
} JanusPnStage_t;

#define FREQUENCIES_PER_STAGE   8
typedef struct {
  uint16_t rollover_index;
  uint16_t buffer_index;
  float energies[FREQUENCIES_PER_STAGE + 1]; // extra 1 to check the next frequency for bleeding
  float snr_score;
  float uncapped_snr_score;
  uint8_t symbols_exceeding_threshold;
} WindowedGoertzel_t;

/* Private define ------------------------------------------------------------*/

#define MIN_SYMBOLS_EXCEEDING_SNR 6
#define SYNC_STAGE_1_STEP         30
#define SYNC_STAGE_1_SUBDIVIDE    16
#define SYNC_STAGE_234_SUBDIVIDE  16
#define STAGE_RESULTS_LEN         512 // exxcessive for poc //((FREQUENCIES_PER_STAGE + 4) * SYNC_STAGE_1_SUBDIVIDE)
#define COARSE_STEP_PRECISION     6

#define TARGET_SNR                (8.0f)

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

extern float window[512];

static const uint32_t janus_pn_32 = 0b10101110110001111100110100100000U;
static uint32_t janus_frequencies[32];
static uint16_t window_offsets[SYNC_STAGE_1_SUBDIVIDE];
static uint16_t samples_per_symbol;
static JanusPnStage_t sync_stage = PN_STAGE_1;
static WindowedGoertzel_t stage_results[STAGE_RESULTS_LEN];
static uint16_t stage_results_head = 0;
static uint16_t stage_results_len = 0;
static uint16_t window_offset_index = 0;
static uint16_t processed_buffer_tail = 0;
static uint16_t processed_buffer_len = 0;

static volatile bool sync_error = false;

/* Private function prototypes -----------------------------------------------*/

static void updateParameters(const DspConfig_t* cfg);
static bool janusPnStep(bool* bit, uint16_t step);
static void fillJanusFrequencies(const DspConfig_t* cfg);
static void fillWindowOffsets(const DspConfig_t* cfg);
static bool janusPnSynchronize(const DspConfig_t* cfg);
static void populateResultsStage1(uint16_t start_index, uint16_t end_index, const DspConfig_t* cfg);
static void scoreOffsets(const DspConfig_t* cfg);
static bool findGlobalMax(const DspConfig_t* cfg);
static void stage1TailIncrement();

/* Exported function definitions ---------------------------------------------*/

bool Sync_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, bool* bit, uint16_t step)
{
  (void)(waveform_step); // Planned use for advanced synchronization techniques
  switch (cfg->sync_method) {
    case NO_SYNC:
      return false;
    case SYNC_PN_32_JANUS:
      return janusPnStep(bit, step);
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

bool Sync_Synchronize(const DspConfig_t* cfg)
{
  updateParameters(cfg);
  switch (cfg->sync_method) {
    case NO_SYNC:
      return Input_DetectMessageStart(cfg);
    case SYNC_PN_32_JANUS:
      return janusPnSynchronize(cfg);
    default:
      return false;
  }
}

void Sync_Reset()
{
  memset(stage_results, 0, sizeof(stage_results));
  stage_results_head = 0;
  stage_results_len = 0;
  window_offset_index = 0;
  processed_buffer_tail = 0;
  processed_buffer_len = 0;
  sync_stage = PN_STAGE_1;
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

bool janusPnStep(bool* bit, uint16_t step)
{
  if (step >= 32) return false;
  *bit = (janus_pn_32 >> (31 - step)) & 1;
  return true;
}

// TODO: change for FSK and consider implications of penalty function
void fillJanusFrequencies(const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < 32; i++) {
    bool bit;
    janusPnStep(&bit, i);
    janus_frequencies[i] = Modulate_GetFhbfskFrequency(bit, i, cfg);
  }
}

void fillWindowOffsets(const DspConfig_t* cfg)
{
  // Adding additional precision when calculating the offsets has an especially
  // high significance when the baud rate is high and/or the samples per symbol
  // is far from a multiple of stage 1 subdivide. Without the additional
  // precision, there can be up to the subdivide/2 missing samples.
  static const uint8_t offsets_precision = 4;
  samples_per_symbol = (uint16_t) ((float) ADC_SAMPLING_RATE / cfg->baud_rate);
  uint32_t offsets_increment = (samples_per_symbol << offsets_precision) / SYNC_STAGE_1_SUBDIVIDE;
  for (uint8_t i = 0; i < SYNC_STAGE_1_SUBDIVIDE; i++) {
    window_offsets[i] = (i * offsets_increment) >> offsets_precision;
  }
}

bool janusPnSynchronize(const DspConfig_t* cfg)
{
  switch (sync_stage) {
    case PN_STAGE_1:
      populateResultsStage1(0, 8, cfg);
      scoreOffsets(cfg);
      return findGlobalMax(cfg);
    case PN_STAGE_2:
    case PN_STAGE_3:
    case PN_STAGE_4:
    case PN_STAGE_COMPLETE:
      break;
    default:
      return false;
  }
  return false;
}

void populateResultsStage1(uint16_t start_index, uint16_t end_index, const DspConfig_t* cfg)
{
  GoertzelInfo_t goertzel_info;
  goertzel_info.buf_len = PROCESSING_BUFFER_SIZE;
  goertzel_info.data_len = samples_per_symbol;
  goertzel_info.window = window;
  goertzel_info.energy_normalization = Demodulate_PowerNormalization();
  goertzel_info.window_size = 512;
  uint32_t f[6];
  goertzel_info.f = f;
  float e_f[6];
  goertzel_info.e_f = e_f;
  while (ADC_InputAvailableSamples() > goertzel_info.data_len) {
    goertzel_info.start_pos = ADC_InputGetTail();
    uint16_t num_frequencies = end_index - start_index + 1;
    while (num_frequencies >= 6) {
      for (uint8_t i = 0; i < 6; i++) {
        f[i] = janus_frequencies[end_index - num_frequencies + 1 + i];
      }
      goertzel_6(&goertzel_info);
      for (uint8_t i = 0; i < 6; i++) {
        stage_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 6;
    }

    while (num_frequencies >= 2) {
      for (uint8_t i = 0; i < 2; i++) {
        f[i] = janus_frequencies[end_index - num_frequencies + 1 + i];
      }
      goertzel_2(&goertzel_info);
      for (uint8_t i = 0; i < 2; i++) {
        stage_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 2;
    }

    while (num_frequencies >= 1) {
      for (uint8_t i = 0; i < 1; i++) {
        f[i] = janus_frequencies[end_index - num_frequencies + 1 + i];
      }
      goertzel_1(&goertzel_info);
      for (uint8_t i = 0; i < 1; i++) {
        stage_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 1;
    }
    stage_results[stage_results_head].buffer_index = goertzel_info.start_pos;
    stage_results[stage_results_head].rollover_index = ADC_TailRolloverCount(false);
    stage_results_head = (stage_results_head + 1) % STAGE_RESULTS_LEN;
    stage_results_len++;
    if (stage_results_len >= STAGE_RESULTS_LEN) {
      sync_error = true;
      return;
    }
    stage1TailIncrement();
  }
}

void scoreOffsets(const DspConfig_t* cfg)
{
  uint16_t results_per_stage = SYNC_STAGE_1_SUBDIVIDE * FREQUENCIES_PER_STAGE;

  if (BackgroundNoise_Ready() == false) {
    stage_results_len = 0;
    return;
  }

  float background_noise = BackgroundNoise_Get();

  while (stage_results_len > results_per_stage) {
    uint16_t base_index = (stage_results_head - stage_results_len) & (STAGE_RESULTS_LEN - 1);
    float snr = 0;
    float uncapped_snr = 0;
    stage_results[base_index].symbols_exceeding_threshold = 0;
    
    for (uint16_t i = 0; i < FREQUENCIES_PER_STAGE; i++) {
      uint16_t freq_index = (base_index + i * SYNC_STAGE_1_SUBDIVIDE) & (STAGE_RESULTS_LEN - 1);
      float frequency_energy = stage_results[freq_index].energies[i];
      // penalty for including the next bin
      frequency_energy -= stage_results[freq_index].energies[i + 1];
      float snr_score = frequency_energy / background_noise;
      snr += MIN(snr_score, TARGET_SNR * 2);
      uncapped_snr += snr_score;
      if (snr_score >= TARGET_SNR) {
        stage_results[base_index].symbols_exceeding_threshold++;
      }
    }
    
    snr /= FREQUENCIES_PER_STAGE;
    uncapped_snr /= FREQUENCIES_PER_STAGE;
    stage_results[base_index].snr_score = snr;
    stage_results[base_index].uncapped_snr_score = uncapped_snr;
    stage_results_len--;
    processed_buffer_len++;
  }
}

bool findGlobalMax(const DspConfig_t* cfg)
{
  const uint16_t margin = 3;
  uint16_t best_index = 0;
  float best_snr = 0.0f;

  if (processed_buffer_len < margin) {
    return false;
  }

  for (uint16_t i = 0; i < processed_buffer_len; i++) {
    uint16_t results_index = (i + processed_buffer_tail) % STAGE_RESULTS_LEN;
    float uncapped_snr = stage_results[results_index].uncapped_snr_score;
    float capped_snr = stage_results[results_index].snr_score;
    uint8_t num_exceeding = stage_results[results_index].symbols_exceeding_threshold;
    if ((uncapped_snr > best_snr) && (num_exceeding >= MIN_SYMBOLS_EXCEEDING_SNR) && (capped_snr >= TARGET_SNR)) {
      best_snr = uncapped_snr;
      best_index = results_index;
    }
  }

  uint16_t results_searched = processed_buffer_len - margin;

  if (best_snr < TARGET_SNR) {
    processed_buffer_len -= results_searched;
    processed_buffer_tail += results_searched;
    processed_buffer_tail %= STAGE_RESULTS_LEN;
    return false;
  }
  uint16_t distance_from_end = (processed_buffer_tail + processed_buffer_len - best_index) % STAGE_RESULTS_LEN;
  // Found a max but it is too close to the end.
  if (distance_from_end <= margin) {
    processed_buffer_len -= results_searched;
    processed_buffer_tail += results_searched;
    processed_buffer_tail %= STAGE_RESULTS_LEN;
    return false;
  }

  uint32_t samples_to_wait = 32 * samples_per_symbol;

  uint64_t current_samples = PROCESSING_BUFFER_SIZE * stage_results[best_index].rollover_index;
  current_samples += stage_results[best_index].buffer_index;

  uint64_t target_total_samples = current_samples + samples_to_wait;

  uint16_t target_rollover = target_total_samples / PROCESSING_BUFFER_SIZE;
  uint16_t target_samples = target_total_samples % PROCESSING_BUFFER_SIZE;

  while (ADC_TailRolloverCount(false) < target_rollover) {
    ADC_InputSetTail(ADC_InputGetHead());
    osDelay(1);
  }
  ADC_InputSetTail(ADC_InputGetHead());
  while (ADC_InputGetHead() < target_samples && (ADC_TailRolloverCount(false) == target_rollover)) {
    ADC_InputSetTail(ADC_InputGetHead());
    osDelay(1);
  }
  ADC_InputSetTail(target_samples);
  return true;
}

void stage1TailIncrement()
{
  uint16_t increment_amount;
  if (window_offset_index == (SYNC_STAGE_1_SUBDIVIDE - 1)) {
    increment_amount = samples_per_symbol - window_offsets[window_offset_index];
  }
  else {
    increment_amount = window_offsets[window_offset_index + 1] - window_offsets[window_offset_index];
  }
  ADC_InputTailAdvance(increment_amount);
  window_offset_index = (window_offset_index + 1) % SYNC_STAGE_1_SUBDIVIDE;
}
