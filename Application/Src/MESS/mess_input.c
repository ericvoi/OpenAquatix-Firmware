/*
 * mess_input.c
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_adc.h"
#include "mess_input.h"
#include "mess_demodulate.h"
#include "mess_packet.h"
#include "mess_main.h"
#include "mess_modulate.h"
#include "mess_error_correction.h"
#include "mess_interleaver.h"
#include "mess_sync.h"
#include "mess_preamble.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "cfg_main.h"
#include "usb_comm.h"
#include "pga113-driver.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include <math.h>
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
    {.raw_amplitude_threshold = 80, .length_us = 2500},
    {.raw_amplitude_threshold = 120, .length_us = 1500}
};

static uint16_t unique_frequency_conditions = sizeof(frequency_thresholds) / sizeof(frequency_thresholds[0]);

static uint16_t max_frequency_threshold_length;

static uint16_t frequency_check_index_0;
static uint16_t frequency_check_index_1;

static MsgStartFunctions_t message_start_function = DEFAULT_MSG_START_FCN;
static bool automatic_gain_control = DEFAULT_AGC_STATE;
static PgaGain_t fixed_pga_gain = DEFAULT_FIXED_PGA_GAIN;

/* Private function prototypes -----------------------------------------------*/

static bool messageStartWithThreshold(void);
static bool messageStartWithFrequency(const DspConfig_t* cfg);
static float frequencyToIndex(float frequency, uint16_t fft_size);
static float indexToFrequency(float index, uint16_t fft_size);
static bool checkFftConditions(uint16_t check_length, float multiplier);
static uint16_t findStartPosition(uint16_t analysis_index, uint16_t check_length);
static bool printReceivedWaveform(char* preamble_sequence);
static void updateFrequencyIndices(const DspConfig_t* cfg);
static uint32_t totalWaitSamples(const DspConfig_t* cfg);

/* Exported function definitions ---------------------------------------------*/

bool Input_Init()
{
  bit_index = 0;
  for (uint8_t i = 0; i < MAX_ANALYSIS_BUFFER_SIZE; i++) {
    analysis_blocks[i].analysis_done = true;
  }

  ADC_InputClear();

  max_frequency_threshold_length = 0;

  for (uint16_t i = 0; i < unique_frequency_conditions; i++) {
    frequency_thresholds[i].energy_threshold = (float) frequency_thresholds[i].raw_amplitude_threshold *
        frequency_thresholds[i].raw_amplitude_threshold * MSG_START_FFT_SIZE / 2.0f;

    uint32_t ns_per_sample = 1000000000 / ADC_SAMPLING_RATE;
    frequency_thresholds[i].num_samples = frequency_thresholds[i].length_us * 1000 / ns_per_sample * FFT_OVERLAP / MSG_START_FFT_SIZE + 1;

    if (frequency_thresholds[i].num_samples > max_frequency_threshold_length) {
      max_frequency_threshold_length = frequency_thresholds[i].num_samples;
    }

    frequency_thresholds[i].hits = 0;
  }

  fft_handle64.fftLenRFFT = MSG_START_FFT_SIZE;
  arm_status ret = arm_rfft_64_fast_init_f32(&fft_handle64);
  if (ret != ARM_MATH_SUCCESS) {
    return false;
  }

  fft_handle128.fftLenRFFT = NOISE_FFT_BLOCK_SIZE;
  ret = arm_rfft_128_fast_init_f32(&fft_handle128);
  if (ret != ARM_MATH_SUCCESS) {
    return false;
  }
  return true;
}

bool Input_DetectMessageStart(const DspConfig_t* cfg)
{
  static bool message_detected = false;
  static uint32_t samples_waited = 0;
  if (message_detected == false) {
    switch (message_start_function) {
      case MSG_START_AMPLITUDE:
        if (messageStartWithThreshold() == true) {
          message_detected = true;
        }
        break;
      case MSG_START_FREQUENCY:
        if (messageStartWithFrequency(cfg) == true) {
          message_detected = true;
        }
        break;
      default:
        if (messageStartWithFrequency(cfg) == true) {
          message_detected = true;
        }
    }
  }
  if (message_detected == true) {
    uint32_t samples_to_wait = totalWaitSamples(cfg);
    if (samples_to_wait == 0) {
      message_detected = false;
      samples_waited = 0;
      return true;
    }
    uint16_t new_samples = ADC_InputAvailableSamples();
    if (new_samples + samples_waited >= samples_to_wait) {
      ADC_InputTailAdvance((uint16_t) (samples_to_wait - samples_waited));
      message_detected = false;
      samples_waited = 0;
      return true;
    }
    ADC_InputTailAdvance(new_samples);
    samples_waited += new_samples;
  }
  return false;
}

// Segments blocks and adds them to array of blocks to be processed
bool Input_SegmentBlocks(const DspConfig_t* cfg)
{
  uint16_t analysis_buffer_length = (uint16_t) ((float) ADC_SAMPLING_RATE / cfg->baud_rate);
  while (ADC_InputAvailableSamples() >= analysis_buffer_length) {

    analysis_count1++;

    uint16_t sync_chips = Sync_NumSteps(cfg);

    uint16_t analysis_index = (analysis_start_index + analysis_length) % MAX_ANALYSIS_BUFFER_SIZE;
    analysis_blocks[analysis_index].buf_len = PROCESSING_BUFFER_SIZE;
    analysis_blocks[analysis_index].data_len = analysis_buffer_length;
    analysis_blocks[analysis_index].data_start_index = ADC_InputGetTail();
    analysis_blocks[analysis_index].chip_index = bit_index + sync_chips;
    analysis_blocks[analysis_index].bit_index = bit_index++;
    analysis_blocks[analysis_index].decoded_bit = false;
    analysis_blocks[analysis_index].analysis_done = false;

    analysis_length++;

    if (analysis_length >= MAX_ANALYSIS_BUFFER_SIZE) {
      return false; // overflow of analysis buffers
    }

    ADC_InputTailAdvance(analysis_buffer_length);
  }
  return true;
}

// looks for an analysis block that have not been analyzed
bool Input_ProcessBlocks(BitMessage_t* bit_msg, const DspConfig_t* cfg)
{
  if (bit_msg == NULL) {
    return false;
  }
  if (bit_msg->fully_received == true) {
    return true;
  }

  while (analysis_length != 0) {
    analysis_count2++;
    if (Demodulate_Perform(&analysis_blocks[analysis_start_index], cfg) == false) {
      return false;
    }
    if (Packet_AddBit(bit_msg, analysis_blocks[analysis_start_index].decoded_bit) == false) {
      return false;
    }

    analysis_start_index = (analysis_start_index + 1) % MAX_ANALYSIS_BUFFER_SIZE;
    if (analysis_length == 0) {
      return false;
    }
    analysis_length--;
  }

  return true;
}

// Goes through message to see if enough bits have been received to decode the
// length and data type
bool Input_DecodeBits(BitMessage_t* bit_msg, const DspConfig_t* cfg, Message_t* msg)
{
  if (bit_msg == NULL) {
    return false;
  }

  if (bit_msg->preamble_received == false) { // Still looking for preamble
    if (bit_msg->bit_count >= bit_msg->preamble.ecc_len) {

      if (Interleaver_Undo(bit_msg, cfg, true) == false) {
        return false;
      }

      if (ErrorCorrection_CheckCorrection(bit_msg, cfg, true,
          &bit_msg->error_preamble, &bit_msg->corrected_error_preamble) == false) {
        return false;
      }

      if (ErrorDetection_CheckDetection(bit_msg, &bit_msg->error_preamble, cfg, true) == false) {
        return false;
      }

      if (Preamble_Decode(bit_msg, msg, cfg) == false) {
        return false;
      }
      bit_msg->final_length = bit_msg->preamble.ecc_len;
      bit_msg->final_length += bit_msg->cargo.ecc_len;

      bit_msg->preamble_received = true;
    }
  }
  return true;
}

void Input_Reset()
{
  ADC_InputClear();
  analysis_start_index = 0;
  analysis_length = 0;
  fft_analysis_index = 0;
  fft_analysis_length = 0;
  bit_index = 0;
}

void Input_PrintNoise()
{
  uint16_t timeout_count = 0;
  while (ADC_InputGetHead() < PRINT_BUFFER_SIZE) {
    osDelay(1);
    if (++timeout_count > 100) return;
  }
  ADC_StopInput();
  char print_buffer[PRINT_CHUNK_SIZE * 7 + 1]; // Accommodates max uint16 length + \r\n + 1
  uint16_t print_index = 0;
  COMM_TransmitData("\b\b\r\n\r\n", 6, COMM_USB);
  // This entire loop does not do any wrap around so it is imperative that the
  // print buffer size does not exceed the processing buffer size
  for (uint16_t i = 0; i < PRINT_BUFFER_SIZE; i += PRINT_CHUNK_SIZE) {
    print_index = 0;

    for (uint16_t j = 0; j < PRINT_CHUNK_SIZE && (i + j) < PRINT_BUFFER_SIZE; j++) {
      print_index += sprintf(&print_buffer[print_index], "%.0f\r\n", ADC_InputGetDataAbsolute(i + j));
    }

    COMM_TransmitData((uint8_t*) print_buffer, print_index, COMM_USB);
  }
  ADC_StartInput();
}

bool Input_PrintWaveform(bool* print_next_waveform, bool fully_received)
{
  if (*print_next_waveform == false) {
    return true;
  }

  static const uint16_t mask = PROCESSING_BUFFER_SIZE - 1;

  static bool previous_fully_received = false;
  static uint32_t message_end_time;

  uint16_t new_length = (ADC_InputGetHead() - print_waveform_start_index) & mask;
  if (new_length > 8000) {
    return false;
  }


  if (new_length < WAVEFORM_PRINT_CHUNK_SIZE_UINT16) {
    return true;
  }

  // Sufficient length to transmit and not on the trail end
  if (fully_received == false) {
    // new data to transmit
    if (printReceivedWaveform(print_waveform_start_sequence) == false) {
      return false;
    }
    previous_fully_received = false;
    return true;
  }

  if (previous_fully_received == false && fully_received == true) {
    message_end_time = osKernelGetTickCount();
  }
  previous_fully_received = fully_received;

  if (printReceivedWaveform(print_waveform_start_sequence) == false) {
    return false;
  }

  uint32_t current_time = osKernelGetTickCount();

  if (current_time - message_end_time >= WAVEFORM_PRINT_EXTRA_DURATION_MS) {
    *print_next_waveform = false; // finished printing
    if (printReceivedWaveform(print_waveform_last_sequence) == false) {
      return false;
    }
  }

  return true;
}

void Input_NoiseFft()
{
  uint16_t timeout_count = 0;
  uint16_t peak_index = 0;
  float peak_magnitude = 0;
  while (ADC_InputGetHead() < NOISE_FFT_SAMPLES) {
    osDelay(1);
    if (++timeout_count > 500) {
      return;
    }
  }
  ADC_StopInput();

  float fft_in_buf[NOISE_FFT_BLOCK_SIZE];
  float fft_out_buf[NOISE_FFT_BLOCK_SIZE];
  float fft_sums[NOISE_FFT_BLOCK_SIZE / 2] = {0.0f};

  for (uint16_t i = 0; i < NOISE_FFT_SAMPLES; i += NOISE_FFT_BLOCK_SIZE) {
    for (uint16_t j = 0; j < NOISE_FFT_BLOCK_SIZE; j++) {
      fft_in_buf[j] = ADC_InputGetData(i + j);
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

  COMM_TransmitData("Frequency, Amplitude\r\n", CALC_LEN, COMM_USB);

  char out_buf[80];
  for (uint16_t i = 0; i < NOISE_FFT_BLOCK_SIZE / 2; i++) {
    sprintf(out_buf, "%.2f, %.2f\r\n", indexToFrequency(i, NOISE_FFT_BLOCK_SIZE), 
        fft_sums[i] / (NOISE_FFT_SAMPLES / NOISE_FFT_BLOCK_SIZE));

    COMM_TransmitData(out_buf, CALC_LEN, COMM_USB);
  }

  sprintf(out_buf, "\r\nPeak frequency: %.2fHz with amplitude %.2f\r\n",
      indexToFrequency(peak_index, NOISE_FFT_BLOCK_SIZE), peak_magnitude);

  COMM_TransmitData(out_buf, CALC_LEN, COMM_USB);
  ADC_StartInput();
}

bool Input_UpdatePgaGain()
{
  // TODO: add automatic gain control
  if (automatic_gain_control == true) return false;

  if (Pga113_GetGain() != fixed_pga_gain) {
    Pga113_SetGain(fixed_pga_gain);
  }

  return true;
}

bool Input_RegisterParams()
{
  uint32_t min = MIN_MSG_START_FCN;
  uint32_t max = MAX_MSG_START_FCN;
  if (Param_Register(PARAM_MSG_START_FCN, "message start function", PARAM_TYPE_UINT8,
                     &message_start_function, sizeof(uint8_t), &min, &max, NULL) == false) {
    return false;
  }

  min = MIN_AGC_STATE;
  max = MAX_AGC_STATE;
  if (Param_Register(PARAM_AGC_ENABLE, "automatic gain control", PARAM_TYPE_UINT8,
                     &automatic_gain_control, sizeof(uint8_t), &min, &max, NULL) == false) {
    return false;
  }
  min = MIN_FIXED_PGA_GAIN;
  max = MAX_FIXED_PGA_GAIN;
  if (Param_Register(PARAM_FIXED_PGA_GAIN, "the fixed PGA gain code", PARAM_TYPE_UINT8,
                     &fixed_pga_gain, sizeof(uint8_t), &min, &max, NULL) == false) {
    return false;
  }

  return true;
}


/* Private function definitions ----------------------------------------------*/

bool messageStartWithThreshold()
{
  if (ADC_InputAvailableSamples() == 0) return false; // no new data to process

  while (ADC_InputAvailableSamples() != 0) {
    if (ADC_InputGetData(0) > AMPLITUDE_THRESHOLD) {
      return true;
    }
    ADC_InputTailAdvance(1);
  }

  return false;
}

bool messageStartWithFrequency(const DspConfig_t* cfg)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;

  if (ADC_InputAvailableSamples() < MSG_START_FFT_SIZE) return false;

  updateFrequencyIndices(cfg);

  do {
    // Prepare buffer
    for (uint16_t i = 0; i < MSG_START_FFT_SIZE; i++) {
      fft_input_buffer[i] = ADC_InputGetData(i);
    }

    arm_rfft_fast_f32(&fft_handle64, fft_input_buffer, fft_output_buffer, 0);

    fft_mag_sq_buffer[0] = fft_output_buffer[0] * fft_output_buffer[0];
    for (uint16_t i = 1; i < MSG_START_FFT_SIZE / 2; i++) {
      float real = fft_output_buffer[2 * i];
      float imag = fft_output_buffer[2 * i + 1];

      fft_mag_sq_buffer[i] = real * real + imag * imag;
    }

    fft_analysis[fft_analysis_index].start_index = ADC_InputGetTail();
    fft_analysis[fft_analysis_index].length = MSG_START_FFT_SIZE;
    // skip the dc component to avoid overwhelming
    arm_mean_f32(&fft_mag_sq_buffer[1], MSG_START_FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].average);
    // skip the dc component since it will always dominate
    arm_max_f32(&fft_mag_sq_buffer[1], MSG_START_FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].maximum, &fft_analysis[fft_analysis_index].max_index);

    fft_analysis[fft_analysis_index].frequency0_amplitude = fft_mag_sq_buffer[frequency_check_index_0];
    fft_analysis[fft_analysis_index].frequency1_amplitude = fft_mag_sq_buffer[frequency_check_index_1];


    fft_analysis_index = (fft_analysis_index + 1) & analysis_mask;
    fft_analysis_length += 1;

    ADC_InputTailAdvance(MSG_START_FFT_SIZE / FFT_OVERLAP);
    if (fft_analysis_length >= FFT_ANALYSIS_BUFF_SIZE) {
      // TODO: log error
      return false;
    }

  } while (ADC_InputAvailableSamples() > MSG_START_FFT_SIZE);

  if (fft_analysis_length < 1) return false;

  for (uint16_t i = 0; i < unique_frequency_conditions; i++) {
    if (checkFftConditions(frequency_thresholds[i].num_samples, frequency_thresholds[i].energy_threshold) == true) {
      frequency_thresholds[i].hits++;
      return true;
    }
  }

  fft_analysis_length = max_frequency_threshold_length - 1;

  return false;
}

float frequencyToIndex(float frequency, uint16_t fft_size)
{
  return frequency * fft_size / ((float) ADC_SAMPLING_RATE);
}

static float indexToFrequency(float index, uint16_t fft_size)
{
  return ADC_SAMPLING_RATE * index / ((float) fft_size);
}

bool checkFftConditions(uint16_t check_length, float multiplier)
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
        ADC_InputSetTail(new_tail);
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

bool printReceivedWaveform(char* preamble_sequence)
{
  if (preamble_sequence == NULL) {
    return false;
  }

  static const uint16_t mask = PROCESSING_BUFFER_SIZE - 1;

  uint16_t out_buffer_index = 0;

  for (; out_buffer_index < WAVEFORM_PRINT_PREAMBLE_SIZE; out_buffer_index++) {
    print_waveform_out_buffer[out_buffer_index] = preamble_sequence[out_buffer_index];
  }

  for (uint16_t i = 0; i < WAVEFORM_PRINT_CHUNK_SIZE_UINT16; i++) {
    // Back converted to u16 so it is easier to transfer over limited data rates
    uint16_t data = (uint16_t) ADC_InputGetDataAbsolute((print_waveform_start_index + i) & mask);
    print_waveform_out_buffer[out_buffer_index++] = data & 0xFF;
    print_waveform_out_buffer[out_buffer_index++] = (data >> 8) & 0xFF;
  }

  for (uint8_t i = 0; i < WAVEFORM_PRINT_PREAMBLE_SIZE; i++) {
    print_waveform_out_buffer[out_buffer_index++] = print_waveform_end_data[i];
  }
  if (out_buffer_index != WAVEFORM_PRINT_BUFFER_SIZE) {
    return false;
  }
  print_waveform_start_index = (print_waveform_start_index + WAVEFORM_PRINT_CHUNK_SIZE_UINT16) & mask;
  COMM_TransmitData(print_waveform_out_buffer, out_buffer_index, COMM_USB);
  return true;
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
}

static uint32_t totalWaitSamples(const DspConfig_t* cfg)
{
  uint16_t num_steps = Sync_NumSteps(cfg);
  return (uint32_t) (((uint32_t) num_steps * ADC_SAMPLING_RATE) / cfg->baud_rate);
}
