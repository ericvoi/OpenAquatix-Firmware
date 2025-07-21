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

typedef struct {
  uint16_t buffer_index;
  uint16_t rollover_count;
  float snr_score;
  float uncapped_snr_score;
  uint8_t symbols_exceeding_threshold;
} Stage2Results_t;

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
static WindowedGoertzel_t stage1_results[STAGE_RESULTS_LEN];
static uint16_t stage_results_head = 0;
static uint16_t stage_results_len = 0;
static uint16_t window_offset_index = 0;
static uint16_t processed_buffer_tail = 0;
static uint16_t processed_buffer_len = 0;

static uint16_t stage1_rollover_index;
static uint16_t stage1_buffer_index;

static uint16_t stage2_offsets[SYNC_STAGE_1_SUBDIVIDE];
static uint8_t stage2_fine_step = 0;
static uint8_t stage2_frequency_index = 0;
static Stage2Results_t stage2_results[SYNC_STAGE_234_SUBDIVIDE];

static volatile bool sync_error = false;

/* Private function prototypes -----------------------------------------------*/

static void updateParameters(const DspConfig_t* cfg);
static bool janusPnStep(bool* bit, uint16_t step);
static void fillJanusFrequencies(const DspConfig_t* cfg);
static void fillWindowOffsets(const DspConfig_t* cfg);
static bool janusPnSynchronize();
static void populateResultsStage1(uint16_t start_index, uint16_t end_index);
static void scoreOffsets();
static void findGlobalMax();
static void stage1TailIncrement();
static void fillStage2Results();
static void waitSynchronizationComplete(uint16_t final_rollover_index, uint16_t final_buffer_index);
static void populateGoertzelInfo(GoertzelInfo_t* goertzel_info);
static bool evaluateStage2Results();

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
  memset(stage1_results, 0, sizeof(stage1_results));
  memset(stage2_results, 0, sizeof(stage2_results));
  stage_results_head = 0;
  stage_results_len = 0;
  window_offset_index = 0;
  processed_buffer_tail = 0;
  processed_buffer_len = 0;
  stage2_fine_step = 0;
  stage2_frequency_index = 0;
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

  // Stage 2 window goes from - 2 / stage1 subdivide : 2 / stage1 subdivide
  // centered around the previous maximum correlation
  uint16_t base_value = samples_per_symbol - ((offsets_increment * 2) >> offsets_precision);
  offsets_increment = (offsets_increment * 4) / SYNC_STAGE_234_SUBDIVIDE;
  for (uint8_t i = 0; i < SYNC_STAGE_234_SUBDIVIDE; i++) {
    // skips the center index that was tested before
    uint16_t index = (i > 7) ? (i + 1) : i;
    stage2_offsets[i] = base_value + ((offsets_increment * index) >> offsets_precision);
  }
}

/**
 * JANUS uses a 32-chip sequence to synchronize the sender and the receiver.
 * The frequencies used are fixed making the synchronization process easier.
 * The synchronization process is split into stages to make the process easier.
 * There are 4 stages in total with each stage looking at 8 bits in the
 * synchronization sequence. The first stage looking at bits 0-7 is the most
 * coarse of the stages and is used to detect when a message has started. The
 * following stages hone in on the start of the message and can also be used
 * to estimate doppler effects. (doppler estiamation not implemented yet)
 */
bool janusPnSynchronize()
{
  switch (sync_stage) {
    case PN_STAGE_1:
      populateResultsStage1(0, 8);
      scoreOffsets();
      findGlobalMax();
      break;
    case PN_STAGE_2:
      fillStage2Results();
      return evaluateStage2Results();
    case PN_STAGE_3:
    case PN_STAGE_4:
    case PN_STAGE_COMPLETE:
      break;
    default:
      return false;
  }
  return false;
}

void populateResultsStage1(uint16_t start_index, uint16_t end_index)
{
  GoertzelInfo_t goertzel_info;
  populateGoertzelInfo(&goertzel_info);
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
        stage1_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 6;
    }

    while (num_frequencies >= 2) {
      for (uint8_t i = 0; i < 2; i++) {
        f[i] = janus_frequencies[end_index - num_frequencies + 1 + i];
      }
      goertzel_2(&goertzel_info);
      for (uint8_t i = 0; i < 2; i++) {
        stage1_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 2;
    }

    while (num_frequencies >= 1) {
      for (uint8_t i = 0; i < 1; i++) {
        f[i] = janus_frequencies[end_index - num_frequencies + 1 + i];
      }
      goertzel_1(&goertzel_info);
      for (uint8_t i = 0; i < 1; i++) {
        stage1_results[stage_results_head].energies[end_index - num_frequencies + 1 + i] = goertzel_info.e_f[i];
      }
      num_frequencies -= 1;
    }
    stage1_results[stage_results_head].buffer_index = goertzel_info.start_pos;
    stage1_results[stage_results_head].rollover_index = ADC_TailRolloverCount(false);
    stage_results_head = (stage_results_head + 1) % STAGE_RESULTS_LEN;
    stage_results_len++;
    if (stage_results_len >= STAGE_RESULTS_LEN) {
      sync_error = true;
      return;
    }
    stage1TailIncrement();
  }
}

void scoreOffsets()
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
    stage1_results[base_index].symbols_exceeding_threshold = 0;
    
    for (uint16_t i = 0; i < FREQUENCIES_PER_STAGE; i++) {
      uint16_t freq_index = (base_index + i * SYNC_STAGE_1_SUBDIVIDE) & (STAGE_RESULTS_LEN - 1);
      float frequency_energy = stage1_results[freq_index].energies[i];
      // penalty for including the next bin
      frequency_energy -= stage1_results[freq_index].energies[i + 1];
      float snr_score = frequency_energy / background_noise;
      snr += MIN(snr_score, TARGET_SNR * 2);
      uncapped_snr += snr_score;
      if (snr_score >= TARGET_SNR) {
        stage1_results[base_index].symbols_exceeding_threshold++;
      }
    }
    
    snr /= FREQUENCIES_PER_STAGE;
    uncapped_snr /= FREQUENCIES_PER_STAGE;
    stage1_results[base_index].snr_score = snr;
    stage1_results[base_index].uncapped_snr_score = uncapped_snr;
    stage_results_len--;
    processed_buffer_len++;
  }
}

void findGlobalMax()
{
  const uint16_t margin = 3;
  uint16_t best_index = 0;
  float best_snr = 0.0f;

  if (processed_buffer_len < margin) {
    return;
  }

  for (uint16_t i = 0; i < processed_buffer_len; i++) {
    uint16_t results_index = (i + processed_buffer_tail) % STAGE_RESULTS_LEN;
    float uncapped_snr = stage1_results[results_index].uncapped_snr_score;
    float capped_snr = stage1_results[results_index].snr_score;
    uint8_t num_exceeding = stage1_results[results_index].symbols_exceeding_threshold;
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
    return;
  }
  uint16_t distance_from_end = (processed_buffer_tail + processed_buffer_len - best_index) % STAGE_RESULTS_LEN;
  // Found a max but it is too close to the end.
  if (distance_from_end <= margin) {
    processed_buffer_len -= results_searched;
    processed_buffer_tail += results_searched;
    processed_buffer_tail %= STAGE_RESULTS_LEN;
    return;
  }

  stage1_rollover_index = stage1_results[best_index].rollover_index;
  stage1_buffer_index = stage1_results[best_index].buffer_index;
  sync_stage = PN_STAGE_2;
  uint16_t new_tail = ((uint32_t) stage1_buffer_index + (FREQUENCIES_PER_STAGE - 1) * samples_per_symbol) % PROCESSING_BUFFER_SIZE;
  new_tail += stage2_offsets[0];
  new_tail %= PROCESSING_BUFFER_SIZE;
  ADC_InputSetTail(new_tail); 
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

void fillStage2Results()
{
  GoertzelInfo_t goertzel_info;
  populateGoertzelInfo(&goertzel_info);

  if (BackgroundNoise_Ready() == false) {
    return;
  }

  float background_noise = BackgroundNoise_Get();

  while (ADC_InputAvailableSamples() > samples_per_symbol) {
    if (stage2_frequency_index == 0) {
      stage2_results[stage2_fine_step].buffer_index = ADC_InputGetTail();
      stage2_results[stage2_fine_step].rollover_count = ADC_TailRolloverCount(false);
    }
    goertzel_info.start_pos = ADC_InputGetTail();
    uint32_t frequencies[2];
    frequencies[0] = janus_frequencies[stage2_frequency_index + FREQUENCIES_PER_STAGE];
    frequencies[1] = janus_frequencies[stage2_frequency_index + FREQUENCIES_PER_STAGE + 1];
    goertzel_info.f = frequencies;
    float e_f[2];
    goertzel_info.e_f = e_f;
    goertzel_2(&goertzel_info);
    float frequency_energy = e_f[0];
    // penalty for including the next bin
    frequency_energy -= e_f[1];
    float uncapped_snr = frequency_energy / background_noise;
    float snr = MIN(uncapped_snr, TARGET_SNR * 2);
    if (snr > TARGET_SNR) {
      stage2_results[stage2_fine_step].symbols_exceeding_threshold++;
    }
    stage2_results[stage2_fine_step].snr_score += snr / FREQUENCIES_PER_STAGE;
    stage2_results[stage2_fine_step].uncapped_snr_score += uncapped_snr / FREQUENCIES_PER_STAGE;
    stage2_fine_step++;
    // Check if done with stage 2
    if ((stage2_fine_step == SYNC_STAGE_234_SUBDIVIDE) && 
        (stage2_frequency_index == FREQUENCIES_PER_STAGE)) {
      break;
    }
    if (stage2_fine_step == SYNC_STAGE_234_SUBDIVIDE) {
      stage2_fine_step = 0;
      stage2_frequency_index++;
      ADC_InputTailAdvance(samples_per_symbol - (stage2_offsets[SYNC_STAGE_234_SUBDIVIDE - 1] - stage2_offsets[0]));
    }
    else {
      ADC_InputTailAdvance(stage2_offsets[stage2_fine_step] - stage2_offsets[stage2_fine_step - 1]);
    }
  }
}

void waitSynchronizationComplete(uint16_t final_rollover_index, uint16_t final_buffer_index)
{
  while (ADC_TailRolloverCount(false) < final_rollover_index) {
    ADC_InputSetTail(ADC_InputGetHead());
    osDelay(1);
  }
  ADC_InputSetTail(ADC_InputGetHead());
  while (ADC_InputGetHead() < final_buffer_index && (ADC_TailRolloverCount(false) == final_rollover_index)) {
    ADC_InputSetTail(ADC_InputGetHead());
    osDelay(1);
  }
  ADC_InputSetTail(final_buffer_index);
}

void populateGoertzelInfo(GoertzelInfo_t* goertzel_info)
{
  goertzel_info->buf_len = PROCESSING_BUFFER_SIZE;
  goertzel_info->data_len = samples_per_symbol;
  goertzel_info->window = window;
  goertzel_info->energy_normalization = Demodulate_PowerNormalization();
  goertzel_info->window_size = 512;
}

bool evaluateStage2Results()
{
  uint16_t best_index = 0;
  float best_snr = 0.0f;

  if (stage2_fine_step != SYNC_STAGE_234_SUBDIVIDE && stage2_frequency_index != FREQUENCIES_PER_STAGE) {
    return false;
  }

  for (uint8_t i = 0; i < SYNC_STAGE_234_SUBDIVIDE; i++) {
    float uncapped_snr = stage2_results[i].uncapped_snr_score;
    float capped_snr = stage2_results[i].snr_score;
    uint8_t num_exceeding = stage2_results[i].symbols_exceeding_threshold;
    if ((uncapped_snr > best_snr) && (num_exceeding >= MIN_SYMBOLS_EXCEEDING_SNR) && (capped_snr >= TARGET_SNR)) {
      best_snr = uncapped_snr;
      best_index = i;
    }
  }

  // failed synchronization at the second stage
  if (best_snr < TARGET_SNR) {
    Sync_Reset();
    return false;
  }

  uint32_t samples_to_wait = 24 * samples_per_symbol;

  uint64_t current_samples = stage2_results[best_index].rollover_count * PROCESSING_BUFFER_SIZE;
  current_samples += stage2_results[best_index].buffer_index;

  uint64_t target_samples = current_samples + samples_to_wait;
  uint16_t final_rollover_count = target_samples / PROCESSING_BUFFER_SIZE;
  uint16_t final_buffer_index = target_samples % PROCESSING_BUFFER_SIZE;

  waitSynchronizationComplete(final_rollover_count, final_buffer_index);
  return true;
}
