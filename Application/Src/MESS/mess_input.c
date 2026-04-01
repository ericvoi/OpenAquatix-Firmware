/*
 * mess_input.c
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_filt_resources.h"
#include "mess_input.h"
#include "mess_demodulate.h"
#include "mess_packet.h"
#include "mess_main.h"
#include "mess_modulate.h"
#include "mess_error_detection.h"
#include "mess_error_correction.h"
#include "mess_interleaver.h"
#include "mess_sync.h"
#include "mess_preamble.h"
#include "filt_main.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "cfg_main.h"
#include "hmi_usb.h"
#include "pga113-driver.h"
#include "error_manager.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float average;
  float maximum;
  float frequency0_amplitude;
  float frequency1_amplitude;
  uint32_t max_index;
  uint32_t length;
  uint16_t start_index;
} FFTInfo_t;

typedef struct {
  uint16_t length_us;
  uint16_t num_samples;

  uint16_t raw_amplitude_threshold;
  float energy_threshold;

  uint32_t hits;
} FrequencyThresholds_t;

/* Private define ------------------------------------------------------------*/

#define PRINT_BUFFER_SIZE         1000
#define PRINT_CHUNK_SIZE          50

#define AMPLITUDE_THRESHOLD       (2500.0f)

#define MSG_START_FFT_SIZE        64
#define FFT_ANALYSIS_BUFF_SIZE    512
#define FFT_OVERLAP               4

// The number of samples to go back when printing the waveform
#define WAVEFORM_BACK_AMOUNT              200
// After a message is fully received, still print another
#define WAVEFORM_PRINT_EXTRA_DURATION_MS  200
#define WAVEFORM_PRINT_CHUNK_SIZE_BYTES   1024
#define WAVEFORM_PRINT_CHUNK_SIZE_UINT16  (WAVEFORM_PRINT_CHUNK_SIZE_BYTES / sizeof(uint16_t))
#define WAVEFORM_PRINT_PREAMBLE_SIZE      4
#define WAVEFORM_PRINT_BUFFER_SIZE        (WAVEFORM_PRINT_CHUNK_SIZE_BYTES + \
                                          WAVEFORM_PRINT_PREAMBLE_SIZE * 2)

#define NOISE_FFT_BLOCK_SIZE              128
// The number of ADC sampels to perform analysis on
#define NOISE_FFT_SAMPLES                 12800 // 100 blocks

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static DemodulationInfo_t analysis_blocks[MAX_ANALYSIS_BUFFER_SIZE];

static volatile uint16_t analysis_count1 = 0;
static volatile uint16_t analysis_count2 = 0;

static volatile uint8_t analysis_start_index = 0;
static volatile uint8_t analysis_length = 0;
static uint16_t bit_index = 0;

static uint16_t print_waveform_start_index = 0;
static uint8_t print_waveform_out_buffer[WAVEFORM_PRINT_BUFFER_SIZE];
static char print_waveform_start_sequence[WAVEFORM_PRINT_PREAMBLE_SIZE] = {'D', 'A', 'T', 'A'};
static char print_waveform_last_sequence[WAVEFORM_PRINT_PREAMBLE_SIZE] = {'T', 'E', 'R', 'M'};
static char print_waveform_end_data[WAVEFORM_PRINT_PREAMBLE_SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};

static float fft_input_buffer[MSG_START_FFT_SIZE] __attribute__((section(".dtcm")));
static float fft_output_buffer[MSG_START_FFT_SIZE] __attribute__((section(".dtcm")));
static float fft_mag_sq_buffer[MSG_START_FFT_SIZE / 2];

static FFTInfo_t fft_analysis[FFT_ANALYSIS_BUFF_SIZE];

static uint16_t fft_analysis_index = 0;
static uint16_t fft_analysis_length = 0;

static arm_rfft_fast_instance_f32 fft_handle64;
arm_rfft_fast_instance_f32 fft_handle128;

static FrequencyThresholds_t frequency_thresholds[] = {
    {.raw_amplitude_threshold = 80 << 4, .length_us = 2500},
    {.raw_amplitude_threshold = 120 << 4, .length_us = 1500}
};

static uint16_t unique_frequency_conditions = sizeof(frequency_thresholds) / sizeof(frequency_thresholds[0]);

static uint16_t max_frequency_threshold_length;

static uint16_t frequency_check_index_0;
static uint16_t frequency_check_index_1;

static MsgStartFunctions_t message_start_function = DEFAULT_MSG_START_FCN;
static bool automatic_gain_control = DEFAULT_AGC_STATE;
static PgaGain_t fixed_pga_gain = DEFAULT_FIXED_PGA_GAIN;
static PreambleErrorBehavior_t preamble_error_behavior = DEFAULT_PREAMBLE_ERROR_BEHAVIOR;

DEFINE_DESC_TABLE(MESSAGE_START_FUNCTION_TABLE, msg_start_function_descriptors)
DEFINE_DESC_TABLE(PGA_GAIN_TABLE, pga_gain_descriptors)
DEFINE_DESC_TABLE(PREAMBLE_ERROR_BEHAVIOR_TABLE, preamble_error_behavior_descriptors)

/* Private function prototypes -----------------------------------------------*/

static void messageStartWithThreshold(Message_t* msg, bool* msg_detected);
static void messageStartWithFrequency(const DspConfig_t* cfg, Message_t* msg, bool* msg_detected);
static float frequencyToIndex(float frequency, uint16_t fft_size);
static float indexToFrequency(float index, uint16_t fft_size);
static bool checkFftConditions(uint16_t check_length, float multiplier, Message_t* msg);
static uint16_t findStartPosition(uint16_t analysis_index, uint16_t check_length);
static void printReceivedWaveform(char* preamble_sequence);
static void updateFrequencyIndices(const DspConfig_t* cfg);
static uint32_t totalWaitSamples(const DspConfig_t* cfg);
static void updateThresholdSamples(void);

/* Exported function definitions ---------------------------------------------*/

void Input_Init()
{
  RETURN_IF_ERROR_PRESENT();
  bit_index = 0;
  for (uint8_t i = 0; i < MAX_ANALYSIS_BUFFER_SIZE; i++) {
    analysis_blocks[i].analysis_done = true;
  }

  MessFiltResources_InputAdcClear();

  updateThresholdSamples();

  fft_handle64.fftLenRFFT = MSG_START_FFT_SIZE;
  arm_status ret = arm_rfft_64_fast_init_f32(&fft_handle64);
  if (ret != ARM_MATH_SUCCESS) 
    REGISTER_ERROR(ERROR_FFT_INITIALIZATION);

  fft_handle128.fftLenRFFT = NOISE_FFT_BLOCK_SIZE;
  ret = arm_rfft_128_fast_init_f32(&fft_handle128);
  if (ret != ARM_MATH_SUCCESS) 
    REGISTER_ERROR(ERROR_FFT_INITIALIZATION);
}

void Input_DetectMessageStart(const DspConfig_t* cfg, Message_t* msg, SyncState_t* sync_state)
{
  static bool message_detected = false;
  static uint32_t samples_waited = 0;
  if (message_detected == false) {
    switch (message_start_function) {
      case MSG_START_AMPLITUDE:
        RETURN_IF_ERROR_PRESENT(messageStartWithThreshold(msg, &message_detected));
        break;
      case MSG_START_FREQUENCY:
        RETURN_IF_ERROR_PRESENT(messageStartWithFrequency(cfg, msg, &message_detected));
        break;
      default:
        REGISTER_ERROR(ERROR_UNHANDLED_CASE);
    }
  }
  if (message_detected == true) {
    uint32_t samples_to_wait = totalWaitSamples(cfg);
    if (samples_to_wait == 0) {
      message_detected = false;
      samples_waited = 0;
      *sync_state = SYNC_SUCCESS;
    }
    uint16_t new_samples = MessFiltResources_AvailableProcessingSamples();
    if (new_samples + samples_waited >= samples_to_wait) {
      MessFiltResources_ProcessingTailAdvance((uint16_t) (samples_to_wait - samples_waited));
      message_detected = false;
      samples_waited = 0;
      *sync_state = SYNC_SUCCESS;
    }
    MessFiltResources_ProcessingTailAdvance(new_samples);
    samples_waited += new_samples;
  }
  *sync_state = SYNC_OK;
}

// Segments blocks and adds them to array of blocks to be processed
void Input_SegmentBlocks(const DspConfig_t* cfg)
{
  RETURN_IF_ERROR_PRESENT();
  uint16_t analysis_buffer_length = (uint16_t) ((float) FILT_GetSamplingRate() / cfg->baud_rate);
  while (MessFiltResources_AvailableProcessingSamples() >= analysis_buffer_length) {

    analysis_count1++;

    uint16_t sync_chips = Sync_NumSteps(cfg);

    uint16_t analysis_index = (analysis_start_index + analysis_length) % MAX_ANALYSIS_BUFFER_SIZE;
    analysis_blocks[analysis_index].buf_len = PROCESSING_BUFFER_SIZE;
    analysis_blocks[analysis_index].data_len = analysis_buffer_length;
    analysis_blocks[analysis_index].data_start_index = MessFiltResources_GetProcessingTail();
    analysis_blocks[analysis_index].chip_index = bit_index + sync_chips;
    analysis_blocks[analysis_index].bit_index = bit_index++;
    analysis_blocks[analysis_index].decoded_bit = false;
    analysis_blocks[analysis_index].analysis_done = false;

    analysis_length++;

    if (analysis_length >= MAX_ANALYSIS_BUFFER_SIZE) 
      REGISTER_ERROR(ERROR_ANALYSIS_BUFFER_OVERFLOW);

    MessFiltResources_ProcessingTailAdvance(analysis_buffer_length);
  }
}

// looks for an analysis block that have not been analyzed
void Input_ProcessBlocks(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  RETURN_IF_ERROR_PRESENT();
  if (bit_msg == NULL) REGISTER_ERROR(ERROR_NULL_PTR);
  if (bit_msg->fully_received == true) return;

  while (analysis_length != 0) {
    analysis_count2++;
    Demodulate_Perform(&analysis_blocks[analysis_start_index], cfg);
    if (Packet_AddBit(bit_msg, analysis_blocks[analysis_start_index].decoded_bit) == false) {
      REGISTER_ERROR(ERROR_EXCEED_BIT_MSG_LEN);
    }

    analysis_start_index = (analysis_start_index + 1) % MAX_ANALYSIS_BUFFER_SIZE;
    analysis_length--;
  }
}

// Goes through message to see if enough bits have been received to decode the
// length and data type
void Input_DecodeBits(BitMessage_t* bit_msg, const DspConfig_t* cfg, Message_t* msg, bool* proceed)
{
  RETURN_IF_ERROR_PRESENT();
  if (bit_msg == NULL) REGISTER_ERROR(ERROR_NULL_PTR);

  if (bit_msg->preamble_received == false) { // Still looking for preamble
    if (bit_msg->bit_count >= bit_msg->preamble.ecc_len) {
      // Message timestamp taken when cargo starts. This approach is not precise
      // and could be improved by considering how far we are into the cargo already
      // in case precision within 1 ms is required. Current approach likely accurate
      // to within 5 ms.
      msg->timestamp = osKernelGetTickCount();

      RETURN_IF_ERROR_PRESENT(Interleaver_Undo(bit_msg, cfg, true));

      RETURN_IF_ERROR_PRESENT(ErrorCorrection_CheckCorrection(bit_msg, cfg, true,
          &bit_msg->error_preamble, &bit_msg->corrected_error_preamble));

      RETURN_IF_ERROR_PRESENT(Preamble_Decode(bit_msg, msg, cfg));

      if (bit_msg->error_preamble == true) {
        if (preamble_error_behavior == PREAMBLE_ERROR_DECODE) {
          *proceed = true;
        }
        else {
          *proceed = false;
        }

        if (preamble_error_behavior == PREAMBLE_ERROR_NOTIFY) {
          osEventFlagsSet(print_event_handle, MESS_DROPPED_PACKET_PREAMBLE);
        }
      }
      bit_msg->final_length = bit_msg->preamble.ecc_len + 
                              bit_msg->cargo.ecc_len;

      bit_msg->preamble_received = true;
    }
  }
}

void Input_Reset()
{
  MessFiltResources_InputAdcClear();
  analysis_start_index = 0;
  analysis_length = 0;
  fft_analysis_index = 0;
  fft_analysis_length = 0;
  bit_index = 0;
}

void Input_PrintNoise()
{
  uint16_t timeout_count = 0;
  while (MessFiltResources_GetInputAdcHead() < PRINT_BUFFER_SIZE) {
    osDelay(1);
    if (++timeout_count > 100) return;
  }
  MessFiltResources_StopInputAdc();
  char print_buffer[PRINT_CHUNK_SIZE * 7 + 1]; // Accommodates max uint16 length + \r\n + 1
  uint16_t print_index = 0;
  COMM_TransmitData("\b\b\r\n\r\n", 6, COMM_USB);
  // This entire loop does not do any wrap around so it is imperative that the
  // print buffer size does not exceed the processing buffer size
  for (uint16_t i = 0; i < PRINT_BUFFER_SIZE; i += PRINT_CHUNK_SIZE) {
    print_index = 0;

    for (uint16_t j = 0; j < PRINT_CHUNK_SIZE && (i + j) < PRINT_BUFFER_SIZE; j++) {
      print_index += sprintf(&print_buffer[print_index], "%.2ef\r\n", MessFiltResources_GetInputDataAbsolute(i + j));
    }

    COMM_TransmitData((uint8_t*) print_buffer, print_index, COMM_USB);
  }
  MessFiltResources_StartInputAdc();
}

// TODO: create another head/tail for this whose validity is dependent on a flag
void Input_PrintWaveform(bool* print_next_waveform, bool fully_received)
{
  RETURN_IF_ERROR_PRESENT();
  if (*print_next_waveform == false) return;

  static bool previous_fully_received = false;
  static uint32_t message_end_time;

  uint16_t new_length = (MessFiltResources_GetInputAdcHead() - print_waveform_start_index) & PROCESSING_BUFFER_MASK;
  if (new_length > 8000)
    REGISTER_ERROR(ERROR_PRINT_WAVEFORM_OVERFLOW);
  


  if (new_length < WAVEFORM_PRINT_CHUNK_SIZE_UINT16) return;

  // Sufficient length to transmit and not on the trail end
  if (fully_received == false) {
    // new data to transmit
    RETURN_IF_ERROR_PRESENT(printReceivedWaveform(print_waveform_start_sequence));
    previous_fully_received = false;
    return;
  }

  if (previous_fully_received == false && fully_received == true) {
    message_end_time = osKernelGetTickCount();
  }
  previous_fully_received = fully_received;

  RETURN_IF_ERROR_PRESENT(printReceivedWaveform(print_waveform_start_sequence));

  uint32_t current_time = osKernelGetTickCount();

  if (current_time - message_end_time >= WAVEFORM_PRINT_EXTRA_DURATION_MS) {
    *print_next_waveform = false; // finished printing
    RETURN_IF_ERROR_PRESENT(printReceivedWaveform(print_waveform_last_sequence));
  }
}

void Input_NoiseFft()
{
  uint16_t timeout_count = 0;
  uint16_t peak_index = 0;
  float peak_magnitude = 0;
  while (MessFiltResources_GetInputAdcHead() < NOISE_FFT_SAMPLES) {
    osDelay(1);
    if (++timeout_count > 500) {
      return;
    }
  }
  MessFiltResources_StopInputAdc();

  float fft_in_buf[NOISE_FFT_BLOCK_SIZE];
  float fft_out_buf[NOISE_FFT_BLOCK_SIZE];
  float fft_sums[NOISE_FFT_BLOCK_SIZE / 2] = {0.0f};

  for (uint16_t i = 0; i < NOISE_FFT_SAMPLES; i += NOISE_FFT_BLOCK_SIZE) {
    for (uint16_t j = 0; j < NOISE_FFT_BLOCK_SIZE; j++) {
      fft_in_buf[j] = MessFiltResources_GetProcessingData(i + j);
    }
    arm_rfft_fast_f32(&fft_handle128, fft_in_buf, fft_out_buf, 0);

    fft_sums[0] += fft_out_buf[0] / NOISE_FFT_BLOCK_SIZE;
    for (uint16_t j = 1; j < NOISE_FFT_BLOCK_SIZE / 2; j++) {
      float real = fft_out_buf[2 * j];
      float imag = fft_out_buf[2 * j + 1];

      float mag = sqrtf(real * real + imag * imag) / NOISE_FFT_BLOCK_SIZE;
      fft_sums[j] += mag;

      if (mag > peak_magnitude) {
        peak_index = j;
        peak_magnitude = mag;
      }
    }
  }

  COMM_TransmitData("\b\b\r\n\r\n", 6, COMM_USB);

  COMM_TransmitData("Frequency Amplitude\r\n", CALC_LEN, COMM_USB);

  char out_buf[80];
  for (uint16_t i = 0; i < NOISE_FFT_BLOCK_SIZE / 2; i++) {
    sprintf(out_buf, "%-9.2f %.2e\r\n", indexToFrequency(i, NOISE_FFT_BLOCK_SIZE), 
        fft_sums[i] / (NOISE_FFT_SAMPLES / NOISE_FFT_BLOCK_SIZE));

    COMM_TransmitData(out_buf, CALC_LEN, COMM_USB);
  }

  sprintf(out_buf, "\r\nPeak frequency: %.2fHz with amplitude %.2e\r\n",
      indexToFrequency(peak_index, NOISE_FFT_BLOCK_SIZE), peak_magnitude);

  COMM_TransmitData(out_buf, CALC_LEN, COMM_USB);
  MessFiltResources_StartInputAdc();
}

void Input_UpdatePgaGain()
{
  RETURN_IF_ERROR_PRESENT();
  // TODO: add automatic gain control
  if (automatic_gain_control == true) REGISTER_ERROR(ERROR_AGC);

  if (Pga113_GetGain() != fixed_pga_gain) {
    Pga113_SetGain(fixed_pga_gain);
  }
}

void Input_RegisterParams()
{
  uint32_t min = MIN_MSG_START_FCN;
  uint32_t max = MAX_MSG_START_FCN;
  if (Param_Register(PARAM_MSG_START_FCN, "message start function", 
                     PARAM_TYPE_ENUM, &message_start_function, sizeof(uint8_t), 
                     &min, &max, NULL, msg_start_function_descriptors
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min = MIN_AGC_STATE;
  max = MAX_AGC_STATE;
  if (Param_Register(PARAM_AGC_ENABLE, "automatic gain control", 
                     PARAM_TYPE_UINT8, &automatic_gain_control, 
                     sizeof(uint8_t), &min, &max, NULL, NULL
                     ) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
  min = MIN_FIXED_PGA_GAIN;
  max = MAX_FIXED_PGA_GAIN;
  if (Param_Register(PARAM_FIXED_PGA_GAIN, "the fixed PGA gain code", 
                     PARAM_TYPE_ENUM, &fixed_pga_gain, sizeof(uint8_t), &min, 
                     &max, NULL, pga_gain_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }

  min = MIN_PREAMBLE_ERROR_BEHAVIOR;
  max = MAX_PREAMBLE_ERROR_BEHAVIOR;
  if (Param_Register(PARAM_PREAMBLE_ERROR_BEHAVIOR, "preamble error behavior", 
                     PARAM_TYPE_ENUM, &preamble_error_behavior, 
                     sizeof(uint8_t), &min, &max, NULL, 
                     preamble_error_behavior_descriptors) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
}


/* Private function definitions ----------------------------------------------*/

void messageStartWithThreshold(Message_t* msg, bool* msg_detected)
{
  if (MessFiltResources_AvailableProcessingSamples() == 0) return; // no new data to process

  while (MessFiltResources_AvailableProcessingSamples() != 0) {
    if (MessFiltResources_GetProcessingData(0) > AMPLITUDE_THRESHOLD) {
      *msg_detected = true;
      msg->doppler_mps = 0.0f;
      msg->snr = 0.0f;
      msg->rx_cyccnt = MessFiltResources_AssociatedCyccnt(
          MessFiltResources_GetProcessingTail(), 
          MessFiltResources_TailRolloverCount(false));
    }
    MessFiltResources_ProcessingTailAdvance(1);
  }
}

// Broken: runs out of analysis buffer size very quickly
void messageStartWithFrequency(const DspConfig_t* cfg, Message_t* msg, bool* msg_detected)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;

  if (MessFiltResources_AvailableProcessingSamples() < MSG_START_FFT_SIZE) return;

  updateFrequencyIndices(cfg);

  do {
    // Prepare buffer
    for (uint16_t i = 0; i < MSG_START_FFT_SIZE; i++) {
      fft_input_buffer[i] = MessFiltResources_GetProcessingData(i);
    }

    arm_rfft_fast_f32(&fft_handle64, fft_input_buffer, fft_output_buffer, 0);

    fft_mag_sq_buffer[0] = fft_output_buffer[0] * fft_output_buffer[0];
    for (uint16_t i = 1; i < MSG_START_FFT_SIZE / 2; i++) {
      float real = fft_output_buffer[2 * i];
      float imag = fft_output_buffer[2 * i + 1];

      fft_mag_sq_buffer[i] = real * real + imag * imag;
    }

    fft_analysis[fft_analysis_index].start_index = MessFiltResources_GetProcessingTail();
    fft_analysis[fft_analysis_index].length = MSG_START_FFT_SIZE;
    // skip the dc component to avoid overwhelming
    arm_mean_f32(&fft_mag_sq_buffer[1], MSG_START_FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].average);
    // skip the dc component since it will always dominate
    arm_max_f32(&fft_mag_sq_buffer[1], MSG_START_FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].maximum, &fft_analysis[fft_analysis_index].max_index);

    fft_analysis[fft_analysis_index].frequency0_amplitude = fft_mag_sq_buffer[frequency_check_index_0];
    fft_analysis[fft_analysis_index].frequency1_amplitude = fft_mag_sq_buffer[frequency_check_index_1];


    fft_analysis_index = (fft_analysis_index + 1) & analysis_mask;
    fft_analysis_length += 1;

    MessFiltResources_ProcessingTailAdvance(MSG_START_FFT_SIZE / FFT_OVERLAP);
    if (fft_analysis_length >= FFT_ANALYSIS_BUFF_SIZE)
      REGISTER_ERROR(ERROR_ANALYSIS_BUFFER_OVERFLOW);

  } while (MessFiltResources_AvailableProcessingSamples() > MSG_START_FFT_SIZE);

  if (fft_analysis_length < 1) return;

  for (uint16_t i = 0; i < unique_frequency_conditions; i++) {
    if (checkFftConditions(frequency_thresholds[i].num_samples, frequency_thresholds[i].energy_threshold, msg) == true) {
      frequency_thresholds[i].hits++;
      *msg_detected = true;
      msg->doppler_mps = 0.0f;
      msg->snr = 0.0f;
      return;
    }
  }

  fft_analysis_length = max_frequency_threshold_length - 1;
}

float frequencyToIndex(float frequency, uint16_t fft_size)
{
  float folded_frequency = FILT_PassbandToBaseband((uint32_t) frequency);
  return folded_frequency * fft_size / ((float) FILT_GetSamplingRate());
}

float indexToFrequency(float index, uint16_t fft_size)
{
  return FILT_GetSamplingRate() * index / ((float) fft_size);
}

bool checkFftConditions(uint16_t check_length, float multiplier, Message_t* msg)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;
  static const uint16_t buffer_mask = PROCESSING_BUFFER_SIZE - 1;
  uint16_t check_count = 0;
  // looks for check_length successive points that meet the threshold condition and then sets the array start location to be at the start of the first in the chain
  for (uint16_t i = max_frequency_threshold_length - check_length; i < fft_analysis_length; i++) {
    uint16_t remaining_length = fft_analysis_length - i;
    if (remaining_length + check_count < check_length) break; // not enough data points left

    uint16_t index = (fft_analysis_index - fft_analysis_length + i) & analysis_mask;
    if ((fft_analysis[index].frequency0_amplitude > multiplier) ||
        (fft_analysis[index].frequency1_amplitude > multiplier)) {
      check_count++;
      if (check_count >= check_length) {
        uint16_t new_tail = findStartPosition((index - check_length + 1) & analysis_mask, check_length);
        MessFiltResources_SetProcessingTail(new_tail);
        msg->rx_cyccnt = MessFiltResources_AssociatedCyccnt(
          MessFiltResources_GetProcessingTail(), 
          MessFiltResources_TailRolloverCount(false));
        print_waveform_start_index = (new_tail - WAVEFORM_BACK_AMOUNT) & buffer_mask;
        return true;
      }
    } else {
      check_count = 0;
    }
  }
  return false;
}

static uint16_t findStartPosition(uint16_t analysis_index, uint16_t check_length)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;
  static const uint16_t buffer_mask = PROCESSING_BUFFER_SIZE - 1;
  if (check_length == 1) {
    return (fft_analysis[analysis_index].start_index + MSG_START_FFT_SIZE / 2) & analysis_mask;
  }

  float first_amplitude;
  float second_amplitude;
  if (fft_analysis[analysis_index].frequency0_amplitude > fft_analysis[analysis_index].frequency1_amplitude) {
    first_amplitude = fft_analysis[analysis_index].frequency0_amplitude;
    second_amplitude = fft_analysis[(analysis_index + 1) & analysis_mask].frequency0_amplitude;
  }
  else {
    first_amplitude = fft_analysis[analysis_index].frequency1_amplitude;
    second_amplitude = fft_analysis[(analysis_index + 1) & analysis_mask].frequency1_amplitude;
  }

  const float ratio_threshold = 1.5 * 1.5;

  // Large increase in the amplitude between successive analysis
  if (second_amplitude / first_amplitude > ratio_threshold) {
    return (fft_analysis[analysis_index].start_index + MSG_START_FFT_SIZE - MSG_START_FFT_SIZE / FFT_OVERLAP) & buffer_mask;
  }
  else {
    return (fft_analysis[analysis_index].start_index + MSG_START_FFT_SIZE / 2) & buffer_mask;
  }
}

void printReceivedWaveform(char* preamble_sequence)
{
  uint16_t out_buffer_index = 0;

  for (; out_buffer_index < WAVEFORM_PRINT_PREAMBLE_SIZE; out_buffer_index++) {
    print_waveform_out_buffer[out_buffer_index] = preamble_sequence[out_buffer_index];
  }

  for (uint16_t i = 0; i < WAVEFORM_PRINT_CHUNK_SIZE_UINT16; i++) {
    // Back converted to u16 so it is easier to transfer over limited data rates
    uint16_t data = (uint16_t) MessFiltResources_GetInputDataAbsolute((print_waveform_start_index + i) & PROCESSING_BUFFER_MASK);
    print_waveform_out_buffer[out_buffer_index++] = data & 0xFF;
    print_waveform_out_buffer[out_buffer_index++] = (data >> 8) & 0xFF;
  }

  for (uint8_t i = 0; i < WAVEFORM_PRINT_PREAMBLE_SIZE; i++) {
    print_waveform_out_buffer[out_buffer_index++] = print_waveform_end_data[i];
  }
  if (out_buffer_index != WAVEFORM_PRINT_BUFFER_SIZE) 
    REGISTER_ERROR(ERROR_SANITY_CHECK);
  
  print_waveform_start_index = (print_waveform_start_index + WAVEFORM_PRINT_CHUNK_SIZE_UINT16) & PROCESSING_BUFFER_MASK;
  COMM_TransmitData(print_waveform_out_buffer, out_buffer_index, COMM_USB);
}

void updateFrequencyIndices(const DspConfig_t* cfg)
{
  static uint32_t previous_version_number = 0; 
  uint32_t current_version_number = CFG_GetVersionNumber(); 

  if (current_version_number == previous_version_number) {
    return; // No updated needed
  }

  previous_version_number = current_version_number;

  uint32_t frequency0, frequency1;

  if (cfg->mod_demod_method == MOD_DEMOD_FSK) {
    frequency0 = cfg->fsk_f0;
    frequency1 = cfg->fsk_f1;
  }
  else {
    frequency0 = Modulate_GetFhbfskFrequency(false, 0, cfg);
    frequency1 = Modulate_GetFhbfskFrequency(true, 0, cfg);
  }

  float index0 = frequencyToIndex(frequency0, MSG_START_FFT_SIZE);
  float index1 = frequencyToIndex(frequency1, MSG_START_FFT_SIZE);

  frequency_check_index_0 = (uint16_t) roundf(index0);
  frequency_check_index_1 = (uint16_t) roundf(index1);

  if (frequency_check_index_0 != frequency_check_index_1) {
    // Sufficient spread in frequency spread indices
    return;
  }

  // From this point onwards, assume that the frequencies are close to one another

  float integral_part; // Ignore the integral part of modff
  float fractional_0 = modff(frequency_check_index_0, &integral_part);
  float fractional_1 = modff(frequency_check_index_1, &integral_part);

  // Additional conditions to increase the spread in frequencies tested

  if (fractional_0 > 0.5 && fractional_1 > 0.5) { // both rounded up
    if (fractional_0 < 0.6) { // but 0 is borderline
      frequency_check_index_0--;
      return;
    }
  }

  if (fractional_0 < 0.5 && fractional_1 < 0.5) { // both rounded down
    if (fractional_1 > 0.4) { // but 1 is borderline
      frequency_check_index_1++;
      return;
    }
  }
  updateThresholdSamples();
}

static uint32_t totalWaitSamples(const DspConfig_t* cfg)
{
  uint16_t num_steps = Sync_NumSteps(cfg);
  return (uint32_t) (((uint32_t) num_steps * FILT_GetSamplingRate()) / cfg->baud_rate);
}

void updateThresholdSamples(void)
{
  static uint32_t last_cfg_num = 0;
  uint32_t current_cfg_num = CFG_GetVersionNumber();
  if (last_cfg_num == current_cfg_num) return;
  last_cfg_num = current_cfg_num;

  max_frequency_threshold_length = 0;

  for (uint16_t i = 0; i < unique_frequency_conditions; i++) {
    frequency_thresholds[i].energy_threshold = (float) frequency_thresholds[i].raw_amplitude_threshold *
        frequency_thresholds[i].raw_amplitude_threshold * MSG_START_FFT_SIZE / 2.0f /
        (1 << (ADC_BITS - 1)) / (1 << (ADC_BITS - 1));

    uint32_t ns_per_sample = 1000000000 / (FILT_GetSamplingRate());
    frequency_thresholds[i].num_samples = frequency_thresholds[i].length_us * 1000 / ns_per_sample * FFT_OVERLAP / MSG_START_FFT_SIZE + 1;

    if (frequency_thresholds[i].num_samples > max_frequency_threshold_length) {
      max_frequency_threshold_length = frequency_thresholds[i].num_samples;
    }

    frequency_thresholds[i].hits = 0;
  }
}
