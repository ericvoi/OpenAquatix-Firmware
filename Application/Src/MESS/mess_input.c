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
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "usb_comm.h"
#include "cmsis_os.h"
#include "arm_math.h"
#include "arm_const_structs.h"
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

/* Private define ------------------------------------------------------------*/

//#define REDUCED_SENSITIVITY

#define PRINT_BUFFER_SIZE         1000
#define PRINT_CHUNK_SIZE          50

#define AMPLITUDE_THRESHOLD       2500

#define FFT_SIZE                  64
#define FFT_ANALYSIS_BUFF_SIZE    512
#define FFT_OVERLAP               4

// TODO: change to be indicative of modulation scheme used
#define FREQUENCY_INDEX_0         16  // 30 kHz
#define FREQUENCY_INDEX_1         17  // 31.875 kHz

#define LEN_10_THRESH             6.0f
#define LEN_5_THRESH              8.0f
#define LEN_3_THRESH              16.0f
#define LEN_1_THRESH              30.0f

#ifdef REDUCED_SENSITIVITY
#define LEN_10_MAG                10000000.0f
#define LEN_6_MAG                 15000000.0f
#else
#define LEN_10_MAG                750000.0f
#define LEN_6_MAG                 1500000.0f
#endif
#define LEN_3_MAG                 2000000.0f
#define LEN_1_MAG                 8000000000.0f

// The number of samples to go back when printing the waveform
#define WAVEFORM_BACK_AMOUNT              200
// After a message is fully received, still print another
#define WAVEFORM_PRINT_EXTRA_DURATION_MS  200
#define WAVEFORM_PRINT_CHUNK_SIZE_BYTES   1024
#define WAVEFORM_PRINT_CHUNK_SIZE_UINT16  (WAVEFORM_PRINT_CHUNK_SIZE_BYTES / sizeof(uint16_t))
#define WAVEFORM_PRINT_PREAMBLE_SIZE      4
#define WAVEFORM_PRINT_BUFFER_SIZE        (WAVEFORM_PRINT_CHUNK_SIZE_BYTES + \
                                          WAVEFORM_PRINT_PREAMBLE_SIZE * 2)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static DemodulationInfo_t analysis_blocks[MAX_ANALYSIS_BUFFER_SIZE];

static uint16_t input_buffer[PROCESSING_BUFFER_SIZE];

static volatile uint16_t buffer_start_index = 0;
static volatile uint16_t buffer_end_index = 0;

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

static float fft_input_buffer[FFT_SIZE];
static float fft_output_buffer[FFT_SIZE];
static float fft_mag_sq_buffer[FFT_SIZE / 2];

static FFTInfo_t fft_analysis[FFT_ANALYSIS_BUFF_SIZE];

static uint16_t fft_analysis_index = 0;
static uint16_t fft_analysis_length = 0;

static arm_rfft_fast_instance_f32 fft_handle;

//static volatile uint32_t len_1_hits = 0;
//static volatile uint32_t len_3_hits = 0;
static volatile uint32_t len_6_hits = 0;
static volatile uint32_t len_10_hits = 0;

static uint32_t print_count = 0;

static MsgStartFunctions_t message_start_function = DEFAULT_MSG_START_FCN;

/* Private function prototypes -----------------------------------------------*/

static uint16_t getBufferLength();
static bool messageStartWithThreshold();
static bool messageStartWithFrequency();
static float indexToFrequency(uint16_t index);
static uint16_t frequencyToIndex(float frequency);
static bool checkFftConditions(const uint16_t check_length, const float multiplier);
static uint16_t findStartPosition(const uint16_t analysis_index, const uint16_t check_length);
static bool printReceivedWaveform(char* preamble_sequence);

/* Exported function definitions ---------------------------------------------*/

bool Input_Init()
{
  buffer_start_index = 0;
  buffer_end_index = 0;
  bit_index = 0;
  for (uint8_t i = 0; i < MAX_ANALYSIS_BUFFER_SIZE; i++) {
    analysis_blocks[i].analysis_done = true;
  }
  if (ADC_RegisterInputBuffer(input_buffer) == false) return false;

  memset(input_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(uint16_t));

  fft_handle.fftLenRFFT = FFT_SIZE;

  arm_status ret = arm_rfft_64_fast_init_f32(&fft_handle);

  return ret == ARM_MATH_SUCCESS;
}

//TODO: add check for overflowing buffer
void Input_IncrementEndIndex()
{
  buffer_end_index = (buffer_end_index + ADC_BUFFER_SIZE / 2) % PROCESSING_BUFFER_SIZE;
}

bool Input_DetectMessageStart()
{
  switch (message_start_function) {
    case MSG_START_AMPLITUDE:
      return messageStartWithThreshold();
      break;
    case MSG_START_FREQUENCY:
      return messageStartWithFrequency();
      break;
    default:
      return messageStartWithFrequency();
  }
}

// Segments blocks and adds them to array of blocks to be processed
bool Input_SegmentBlocks()
{
  uint32_t analysis_buffer_length = ADC_SAMPLING_RATE / baud_rate;
  while (getBufferLength() >= analysis_buffer_length) {

    analysis_count1++;

    uint16_t analysis_index = (analysis_start_index + analysis_length) % MAX_ANALYSIS_BUFFER_SIZE;
    analysis_blocks[analysis_index].data_buf = input_buffer;
    analysis_blocks[analysis_index].buf_len = PROCESSING_BUFFER_SIZE;
    analysis_blocks[analysis_index].data_len = analysis_buffer_length;
    analysis_blocks[analysis_index].data_start_index = buffer_start_index;
    analysis_blocks[analysis_index].bit_index = bit_index++;
    analysis_blocks[analysis_index].decoded_bit = false;
    analysis_blocks[analysis_index].analysis_done = false;

    analysis_length++;

    if (analysis_length >= MAX_ANALYSIS_BUFFER_SIZE) {
      return false; // overflow of analysis buffers
    }

    buffer_start_index = (buffer_start_index + analysis_buffer_length) % PROCESSING_BUFFER_SIZE;
  }
  return true;
}

// looks for an analysis block that have not been analyzed
// TODO: Address possibility for overflow in eval_info
// The eval_info arrays need bounds checking against bit_msg->bit_count
// to prevent buffer overrun when processing long messages
bool Input_ProcessBlocks(BitMessage_t* bit_msg, EvalMessageInfo_t* eval_info)
{
  if (bit_msg == NULL || eval_info == NULL) {
    return false;
  }
  if (bit_msg->fully_received == true) {
    return true;
  }

  while (analysis_length != 0) {
    analysis_count2++;
    if (Demodulate_Perform(&analysis_blocks[analysis_start_index]) == false) {
      return false;
    }
    if (Packet_AddBit(bit_msg, analysis_blocks[analysis_start_index].decoded_bit) == false) {
      return false;
    }
    eval_info->energy_f0[bit_msg->bit_count - 1] = analysis_blocks[analysis_start_index].energy_f0;
    eval_info->energy_f1[bit_msg->bit_count - 1] = analysis_blocks[analysis_start_index].energy_f1;
    eval_info->f0[bit_msg->bit_count - 1] = analysis_blocks[analysis_start_index].f0;
    eval_info->f1[bit_msg->bit_count - 1] = analysis_blocks[analysis_start_index].f1;

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
bool Input_DecodeBits(BitMessage_t* bit_msg, bool evaluation_mode)
{
  if (bit_msg == NULL) {
    return false;
  }
  // There is no header or preamble in evaluation mode.
  if (evaluation_mode == true) {
    bit_msg->final_length = EVAL_MESSAGE_LENGTH;
    return true;
  }

  if (bit_msg->preamble_received == false) { // Still looking for preamble
    if (bit_msg->bit_count >= PACKET_PREAMBLE_LENGTH_BITS) {
      // Keeps track of where in the preamble we are
      uint16_t bit_index = 0;
      // The first set of bytes in the message correspond to the sender's id
      if (Packet_Get8BitChunk(bit_msg, &bit_index, PACKET_SENDER_ID_BITS,
          &bit_msg->sender_id) == false) {
        return false;
      }
      // The second set of bytes in the message correspond to the data type
      if (Packet_Get8BitChunk(bit_msg, &bit_index, PACKET_MESSAGE_TYPE_BITS,
          &bit_msg->contents_data_type) == false) {
        return false;
      }
      uint8_t packet_length;
      // The third set of bytes in the message correspond to the data length
      if (Packet_Get8BitChunk(bit_msg, &bit_index, PACKET_LENGTH_BITS,
          &packet_length) == false) {
        return false;
      }
      bit_msg->data_len_bits = 8 << packet_length;
      bit_msg->final_length = 8 << packet_length;
      bit_msg->final_length += PACKET_PREAMBLE_LENGTH_BITS;
      uint16_t error_bits_length;
      if (ErrorCorrection_CheckLength(&error_bits_length) == false) {
        return false;
      }
      bit_msg->final_length += error_bits_length;
      // The fourth set of bytes in the message correspond to the stationary flag
      if (Packet_Get8BitChunk(bit_msg, &bit_index, PACKET_STATIONARY_BITS,
          (uint8_t*) &bit_msg->stationary_flag) == false) {
        return false;
      }

      // Asserts that the amount of bits read == the amount of bits in the preamble
      if (bit_index != PACKET_PREAMBLE_LENGTH_BITS) {
        return false;
      }

      bit_msg->preamble_received = true;
    }
  }
  return true;
}

// copies data from the bit message to the actual message
bool Input_DecodeMessage(BitMessage_t* input_bit_msg, Message_t* msg)
{
  if (input_bit_msg == NULL || msg == NULL) {
    return false;
  }
  // data_len_bytes is restricted to be a multiple of 8
  uint16_t len_bytes = input_bit_msg->data_len_bits / 8;

  uint16_t start_position = PACKET_PREAMBLE_LENGTH_BITS;

  for (uint16_t i = 0; i < len_bytes; i++) {
    if (Packet_Get8(input_bit_msg, &start_position, msg->data + i) == false) {
      return false;
    }
  }
  return true;
}

void Input_Reset()
{
  buffer_start_index = 0;
  buffer_end_index = 0;
  analysis_start_index = 0;
  analysis_length = 0;
  fft_analysis_index = 0;
  fft_analysis_length = 0;
  bit_index = 0;
  memset(input_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(uint16_t));
}

void Input_PrintNoise()
{
  char print_buffer[PRINT_CHUNK_SIZE * 7 + 1]; // Accommodates max uint16 length + \r\n + 1
  uint16_t print_index = 0;
  COMM_TransmitData("\b\b\r\n\r\n", 6, COMM_USB);
  for (uint16_t i = 0; i < PRINT_BUFFER_SIZE; i += PRINT_CHUNK_SIZE) {
    print_index = 0;

    for (uint16_t j = 0; j < PRINT_CHUNK_SIZE && (i + j) < PRINT_BUFFER_SIZE; j++) {
      print_index += sprintf(&print_buffer[print_index], "%u\r\n", input_buffer[i + j]);
    }


    COMM_TransmitData((uint8_t*) print_buffer, print_index, COMM_USB);
  }
}

bool Input_PrintWaveform(bool* print_next_waveform, bool fully_received)
{
  if (*print_next_waveform == false) {
    return true;
  }

  static const uint16_t mask = PROCESSING_BUFFER_SIZE - 1;

  static bool previous_fully_received = false;
  static uint32_t message_end_time;

  uint16_t new_length = (buffer_end_index - print_waveform_start_index) & mask;
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

bool Input_RegisterParams()
{
  uint32_t min = MIN_MSG_START_FCN;
  uint32_t max = MAX_MSG_START_FCN;
  if (Param_Register(PARAM_MSG_START_FCN, "message start function", PARAM_TYPE_UINT8,
                     &message_start_function, sizeof(uint8_t), &min, &max) == false) {
    return false;
  }

  return true;
}


/* Private function definitions ----------------------------------------------*/

static uint16_t getBufferLength()
{
  uint16_t buffer_length;

  if (buffer_end_index >= buffer_start_index) {
    buffer_length = buffer_end_index - buffer_start_index;
  }
  else {
    buffer_length = PROCESSING_BUFFER_SIZE - (buffer_start_index - buffer_end_index);
  }
  return buffer_length;
}

bool messageStartWithThreshold()
{
  uint16_t end_index = buffer_end_index;
  if (buffer_start_index == buffer_end_index) return false; // no new data to process

  static const uint16_t mask = PROCESSING_BUFFER_SIZE - 1;

  while (buffer_start_index != end_index) {
    if (input_buffer[buffer_start_index] > AMPLITUDE_THRESHOLD) {
      return true;
    }
    buffer_start_index = (buffer_start_index + 1) & mask;
  }

  return false;
}

bool messageStartWithFrequency()
{
  static const uint16_t buffer_mask = PROCESSING_BUFFER_SIZE - 1;
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;
  uint16_t end_index = buffer_end_index;

  uint16_t difference = (end_index - buffer_start_index) & buffer_mask;

  if (difference < FFT_SIZE) return false;

  do {
    // Prepare buffer
    for (uint16_t i = 0; i < FFT_SIZE; i++) {
      fft_input_buffer[i] = (float) input_buffer[(buffer_start_index + i) & buffer_mask];
    }

    arm_rfft_fast_f32(&fft_handle, fft_input_buffer, fft_output_buffer, 0);



    fft_mag_sq_buffer[0] = fft_output_buffer[0] * fft_output_buffer[0];
    for (uint16_t i = 1; i < FFT_SIZE / 2; i++) {
      float real = fft_output_buffer[2 * i];
      float imag = fft_output_buffer[2 * i + 1];

      fft_mag_sq_buffer[i] = real * real + imag * imag;
    }

    fft_analysis[fft_analysis_index].start_index = buffer_start_index;
    fft_analysis[fft_analysis_index].length = FFT_SIZE;
    // skip the dc component to avoid overwhelming
    arm_mean_f32(&fft_mag_sq_buffer[1], FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].average);
    // skip the dc component since it will always dominate
    arm_max_f32(&fft_mag_sq_buffer[1], FFT_SIZE / 2 - 1, &fft_analysis[fft_analysis_index].maximum, &fft_analysis[fft_analysis_index].max_index);

    fft_analysis[fft_analysis_index].frequency0_amplitude = fft_mag_sq_buffer[FREQUENCY_INDEX_0];
    fft_analysis[fft_analysis_index].frequency1_amplitude = fft_mag_sq_buffer[FREQUENCY_INDEX_1];


    fft_analysis_index = (fft_analysis_index + 1) & analysis_mask;
    fft_analysis_length += 1;

    buffer_start_index = (buffer_start_index + FFT_SIZE / FFT_OVERLAP) & buffer_mask;
    if (fft_analysis_length >= FFT_ANALYSIS_BUFF_SIZE) {
      // TODO: log error
      return false;
    }

    difference = (end_index - buffer_start_index) & buffer_mask;

  } while (difference > FFT_SIZE);

  // go through the fft_analysis array to see if the condition is met

  // TODO later add individual start indices for each

  if (fft_analysis_length < 1) return false;

//  if (checkFftConditions(1, LEN_1_MAG) == true) {
//    len_1_hits++;
//    return true;
//  }

//  if (checkFftConditions(3, LEN_3_MAG) == true) {
//    len_3_hits++;
//    return true;
//  }

  if (checkFftConditions(6, LEN_6_MAG) == true) {
    len_6_hits++;
    return true;
  }

  if (checkFftConditions(10, LEN_10_MAG) == true) {
    len_10_hits++;
    return true;
  }

  fft_analysis_length = 10 - 1;

  return false;
}

float indexToFrequency(uint16_t index)
{
  return (float) (index * (ADC_SAMPLING_RATE / FFT_SIZE));
}

uint16_t frequencyToIndex(float frequency)
{
  return (uint16_t) roundf(frequency * FFT_SIZE / ((float) ADC_SAMPLING_RATE));
}

bool checkFftConditions(const uint16_t check_length, const float multiplier)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;
  static const uint16_t buffer_mask = PROCESSING_BUFFER_SIZE - 1;
  uint16_t check_count = 0;
  // looks for check_length successive points that meet the threshold condition and then sets the array start location to be at the start of the first in the chain
  for (uint16_t i = 10 - check_length; i < fft_analysis_length; i++) {
    uint16_t remaining_length = fft_analysis_length - i;
    if (remaining_length + check_count < check_length) break; // not enough data points left

    uint16_t index = (fft_analysis_index - fft_analysis_length + i) & analysis_mask;
    if ((fft_analysis[index].frequency0_amplitude > multiplier) ||
        (fft_analysis[index].frequency1_amplitude > multiplier)) {
      check_count++;
      if (check_count >= check_length) {
        buffer_start_index = findStartPosition((index - check_length + 1) & analysis_mask, check_length);
        print_waveform_start_index = (buffer_start_index - WAVEFORM_BACK_AMOUNT) & buffer_mask;
        return true;
      }
    } else {
      check_count = 0;
    }
  }
  return false;
}

static uint16_t findStartPosition(const uint16_t analysis_index, const uint16_t check_length)
{
  static const uint16_t analysis_mask = FFT_ANALYSIS_BUFF_SIZE - 1;
  static const uint16_t buffer_mask = PROCESSING_BUFFER_SIZE - 1;
  if (check_length == 1) {
    return (fft_analysis[analysis_index].start_index + FFT_SIZE / 2) & analysis_mask;
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
    return (fft_analysis[analysis_index].start_index + FFT_SIZE - FFT_SIZE / FFT_OVERLAP) & buffer_mask;
  }
  else {
    return (fft_analysis[analysis_index].start_index + FFT_SIZE / 2) & buffer_mask;
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
    uint16_t data = input_buffer[(print_waveform_start_index + i) & mask];
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
  print_count++;
  COMM_TransmitData(print_waveform_out_buffer, out_buffer_index, COMM_USB);
  return true;
}
