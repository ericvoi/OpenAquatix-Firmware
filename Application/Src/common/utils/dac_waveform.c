/*
 * sine_lut.c
 *
 *  Created on: Feb 5, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "dac_waveform.h"
#include "mess_adc.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <string.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint32_t phase_accumulator;
  uint32_t phase_increment;
  uint32_t current_amplitude;
  uint32_t target_amplitude;
  int32_t amplitude_step;
  uint32_t amplitude_counter;
  bool amplitude_transitioning;
} WaveformControl_t;

typedef enum {
  FILL_FIRST_HALF,
  FILL_LAST_HALF
} FillType_t;

/* Private define ------------------------------------------------------------*/

#define SINE_POINTS         1024
#define DAC_MAX_VALUE       4095
#define PHASE_PRECISION     32
#define AMPLITUDE_STEPS     32  // Number of steps for amplitude transition

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t sine_table[SINE_POINTS];
static uint32_t dac_buffer[DAC_BUFFER_SIZE];

static WaveformControl_t wave_ctrl;
static const WaveformStep_t* current_sequence = NULL;
static uint32_t sequence_length = 0;
static uint32_t current_step = 0;
static volatile bool dac_running = false;
static uint32_t current_symbol_duration_us = 0;

static volatile uint32_t callback_count = 0;

/* Private function prototypes -----------------------------------------------*/

static void generateSineTable(void);
static void updateWaveformParameters(const WaveformStep_t* step);
static void fillDacBuffer(FillType_t type);
static void halfFullDmaCallback(void);
static void fullDmaCallback(void);

/* Exported function definitions ---------------------------------------------*/

bool DAC_InitWaveformGenerator(void)
{
  generateSineTable();

  // Initialize control structure
  memset(&wave_ctrl, 0, sizeof(wave_ctrl));
  wave_ctrl.current_amplitude = 0;

  // Configure DAC and DMA here
  HAL_StatusTypeDef ret1 = HAL_TIM_Base_Start(&htim6);

  return (ret1 == HAL_OK);
}

bool DAC_SetWaveformSequence(WaveformStep_t* sequence, uint32_t num_steps)
{
  if(! sequence || num_steps == 0) return false;

  // Length of sequence must be a multiple of half the DAC buffer size
  uint32_t length_multiple = DAC_BUFFER_SIZE / 2;
  // Converts symbol length multiple into micro seconds
  uint32_t length_multiple_us = length_multiple * DAC_SAMPLE_RATE / 1000000;

  // Every symbol duration must be a multiple of half the DAC buffer duration in micro seconds
  for (uint32_t i = 0; i < num_steps; i++) {
    if ((sequence[i].duration_us % length_multiple_us) != 0) {
      return false;
    }

    // Calculate phase increment for each step
    // Phase increment determines how quickly we move through the sine table
    // Formula: phase_inc = (freq * 2^PHASE_PRECISION) / sample_rate
    sequence[i].phase_increment =
        (((uint64_t) sequence[i].freq_hz) << PHASE_PRECISION) / DAC_SAMPLE_RATE;
  }

  current_sequence = sequence;
  sequence_length = num_steps;
  current_step = 0;

  return true;
}

bool DAC_StartWaveformOutput(uint32_t channel)
{
  if (current_sequence == NULL) return false;

  dac_running = true;
  updateWaveformParameters(&current_sequence[0]);
  fillDacBuffer(FILL_FIRST_HALF);
  fillDacBuffer(FILL_LAST_HALF);

  if (channel == DAC_CHANNEL_2) {
    HAL_TIM_Base_Start(&htim6);
  }

  HAL_StatusTypeDef ret = HAL_DAC_Start_DMA(&hdac1, channel, (uint32_t*) dac_buffer,
                    DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);

  return ret == HAL_OK;
}

bool DAC_StopWaveformOutput(void)
{
  // reset flags and end DMA transfer to ease DMA channels
  dac_running = false;
  // current_sequence = NULL; // deprecated
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
  ADC_StopFeedback();
  return true;
}

bool DAC_IsRunning()
{
  return dac_running;
}

/* Private function definitions ----------------------------------------------*/

// Creates a sine table with 360/SINE_POINTS degree spacing between adjacent points centered at 2047. Table has one full sine wave
static void generateSineTable(void)
{
  for(uint16_t i = 0; i < SINE_POINTS; i++) {
    sine_table[i] = (uint16_t)(2047.0f * sinf(2.0f * M_PI * i / SINE_POINTS) + 2047.0f);
  }
}

static void updateWaveformParameters(const WaveformStep_t* step)
{
  // Calculate new phase increment
  wave_ctrl.phase_increment = step->phase_increment;

  // Setup amplitude transition
  wave_ctrl.target_amplitude = (uint32_t) (step->relative_amplitude * (float) DAC_MAX_VALUE);
  wave_ctrl.amplitude_step = ((int32_t) wave_ctrl.target_amplitude - (int32_t) wave_ctrl.current_amplitude) / AMPLITUDE_STEPS;
  wave_ctrl.amplitude_counter = 0;
  wave_ctrl.amplitude_transitioning = true;

  current_symbol_duration_us = 0;
}

static void fillDacBuffer(FillType_t type)
{
  // Flag that indicates that the next time this function is called it should terminate the DAC output
  static bool last_fill = false;

  callback_count++;

  if (last_fill == true) {
    last_fill = false;
    DAC_StopWaveformOutput();
    return;
  }

  // Final step check
  if (current_step == (sequence_length - 1)) {
    if (current_symbol_duration_us >= current_sequence[current_step].duration_us) {
      last_fill = true;
      return;
    }
  }

  // Running index to use
  uint16_t i = (type == FILL_FIRST_HALF) ? 0 : DAC_BUFFER_SIZE / 2;

  const uint16_t start_index = i; // Absolute starting index to use
  const uint16_t end_index = (type == FILL_FIRST_HALF) ? DAC_BUFFER_SIZE / 2: DAC_BUFFER_SIZE;

  if (current_symbol_duration_us >= current_sequence[current_step].duration_us) { // Current sequence step has gone on long enough
    // start new symbol
    current_step++;
    updateWaveformParameters(&current_sequence[current_step]);
  }

  // Flag to change the output frequency has been set so perform amplitude transition
  if (wave_ctrl.amplitude_transitioning) {
    for (;i < start_index + AMPLITUDE_STEPS; i++) {
      // Take the first 10 bits of the phase as the sine table has 2^10 points
      uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
      uint32_t base_value = sine_table[index & (SINE_POINTS - 1)]; // Ensures nothing out of index

      wave_ctrl.current_amplitude = (uint32_t) ((int32_t) wave_ctrl.current_amplitude + wave_ctrl.amplitude_step);
      wave_ctrl.amplitude_counter++;
      if(wave_ctrl.amplitude_counter >= AMPLITUDE_STEPS) {
        wave_ctrl.amplitude_transitioning = false;
        wave_ctrl.current_amplitude = wave_ctrl.target_amplitude;
      }
      // Add baseline offset and modulation amplitude
      dac_buffer[i] = (DAC_MAX_VALUE + 1) / 2 - wave_ctrl.current_amplitude / 2 + ((base_value * wave_ctrl.current_amplitude) >> 12);

      // Update phase
      wave_ctrl.phase_accumulator += wave_ctrl.phase_increment;
    }
  }

  uint16_t offset_amt = (DAC_MAX_VALUE + 1) / 2 - wave_ctrl.current_amplitude / 2;
  for(; i < end_index; i++) {
    // Get current phase
    uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
    uint32_t base_value = sine_table[index & (SINE_POINTS - 1)];

    // Scale output and write to buffer
    dac_buffer[i] = offset_amt + ((base_value * wave_ctrl.current_amplitude) >> 12);

    // Update phase
    wave_ctrl.phase_accumulator += wave_ctrl.phase_increment;
  }

  current_symbol_duration_us += DAC_BUFFER_SIZE * DAC_SAMPLE_RATE / 1000000 / 2;
}

// DMA callbacks
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  halfFullDmaCallback();
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  fullDmaCallback();
}

void HAL_DACEx_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  halfFullDmaCallback();
}

void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  fullDmaCallback();
}

static void halfFullDmaCallback(void)
{
  if(dac_running == true) {
    fillDacBuffer(FILL_FIRST_HALF);
  }
}

static void fullDmaCallback(void)
{
  if(dac_running == true) {
    fillDacBuffer(FILL_LAST_HALF);
  }
}

void HAL_DAC_ErrorCallbackCh2(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
//  uint32_t dma_error = hdac->DMA_Handle1->ErrorCode;
//  uint32_t dac_error = hdac->ErrorCode;

}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
  (void)(hdma);
    // Break here or log error
}
