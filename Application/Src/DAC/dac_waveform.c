/*
 * dac_waveform.c
 *
 *  Created on: Feb 5, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "dac_waveform.h"
#include "dac_main.h"
#include "mess_adc.h"
#include "mess_modulate.h"
#include "mess_dac_resources.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "sleep/wakeup_tones.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint32_t phase_accumulator;
  uint32_t phase_increment;

  uint32_t initial_tukey_window_index;
  uint32_t initial_tukey_end_index;
  uint32_t tukey_increment;
  uint32_t final_tukey_window_index;
  uint32_t final_tukey_start_index;

  uint32_t amplitude;
} WaveformControl_t;

/* Private define ------------------------------------------------------------*/

#define SINE_POINTS         1024
#define DAC_MAX_VALUE       4095
#define PHASE_PRECISION     32

#define TUKEY_PRECISION     10
#define TUKEY_POINTS        (1 << 8) // 256 points in pre-computed Tukey array

#define TUKEY_ALPHA         (0.05f)

#define AMPLITUDE_PRECISION 10

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)          ((a < b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

extern osThreadId_t dacTaskHandle;

static uint16_t sine_table[SINE_POINTS];
static uint32_t dac_buffer[DAC_BUFFER_SIZE] __attribute__((section(".dma_buf")));

static WaveformControl_t wave_ctrl __attribute__((section(".dtcm")));
static WaveformStep_t current_waveform_step;
static volatile uint32_t sequence_length = 0;
static volatile uint16_t current_step = 0;
static volatile bool dac_running = false;
static uint32_t current_symbol_duration_us = 0;

static volatile uint32_t callback_count = 0;

static uint16_t tukey_window[TUKEY_POINTS];

// Output tone that flushes out the DAC and prevents the first message from being scrambled
WaveformStep_t test_step = {
    .output_type = OUTPUT_CONSTANT_SQUARE,
    .duration_us = 1000000, // Any lower duration does not work
    .freq_hz = 30000,
    .relative_amplitude = 0.0
};

/* Private function prototypes -----------------------------------------------*/

static void generateSineTable(void);
static void generateTukeyWindow(void);
static void updateWaveformParameters(void);

/* Exported function definitions ---------------------------------------------*/

bool Waveform_InitWaveformGenerator(void)
{
  generateSineTable();
  generateTukeyWindow();

  // Initialize control structure
  memset(&wave_ctrl, 0, sizeof(wave_ctrl));
  callback_count = 0;
  current_step = 0;
  dac_running = false;
  sequence_length = 0;
  current_symbol_duration_us = 0;

  // Configure DAC and DMA here
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_FEEDBACK);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_TRANSDUCER);

  return true;
}

bool Waveform_SetWaveformSequence(uint16_t num_steps, bool is_message)
{
  if (num_steps == 0) return false;

  uint16_t extra_steps = is_message ? MessDacResource_SyncWakeupSteps() : 0;

  sequence_length = num_steps + extra_steps;
  current_step = 0;

  return true;
}

bool Waveform_StartWaveformOutput(uint32_t channel)
{
  HAL_DAC_Stop_DMA(&hdac1, channel);
  wave_ctrl.phase_accumulator = 0;

  dac_running = true;
  updateWaveformParameters();
  Waveform_FillBuffer(FILL_FIRST_HALF);
  Waveform_FillBuffer(FILL_LAST_HALF);

  HAL_StatusTypeDef ret = HAL_DAC_Start_DMA(&hdac1, channel, (uint32_t*) dac_buffer,
                    DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);

  return ret == HAL_OK;
}

bool Waveform_StopWaveformOutput()
{
  // reset flags and end DMA transfer to ease DMA channels
  dac_running = false;
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_FEEDBACK);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_TRANSDUCER);

  wave_ctrl.phase_accumulator = 0;

  ADC_StopFeedback();
  return true;
}

bool Waveform_IsRunning()
{
  return dac_running;
}

bool Waveform_RegisterParams()
{

  return true;
}

void Waveform_Flush()
{
  Waveform_SetWaveformSequence(1, false);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_FEEDBACK);
  wave_ctrl.phase_accumulator = 0;

  dac_running = true;
  wave_ctrl.phase_increment = 0;

  wave_ctrl.amplitude = test_step.relative_amplitude * (1 << AMPLITUDE_PRECISION);
  wave_ctrl.tukey_increment = 0;
  wave_ctrl.initial_tukey_end_index = 0;
  wave_ctrl.final_tukey_start_index = UINT_MAX;

  current_symbol_duration_us = 0;
  memcpy(&current_waveform_step, &test_step, sizeof(WaveformStep_t));
  Waveform_FillBuffer(FILL_FIRST_HALF);
  Waveform_FillBuffer(FILL_LAST_HALF);

  HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_FEEDBACK, (uint32_t*) dac_buffer,
      DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);

  HAL_TIM_Base_Start(&htim6);

  while (dac_running == true) {
    osDelay(1);
  }
  HAL_TIM_Base_Stop(&htim6);
}

void Waveform_FillBuffer(FillType_t type)
{
  // Flag that indicates that the next time this function is called it should terminate the DAC output
  static bool last_fill = false;

  callback_count++;

  if (last_fill == true) {
    last_fill = false;
    Waveform_StopWaveformOutput();
    return;
  }

  // Final step check
  if (current_step == (sequence_length - 1)) {
    if (current_symbol_duration_us >= current_waveform_step.duration_us) {
      last_fill = true;
      return;
    }
  }

  // Running index to use
  uint16_t i = (type == FILL_FIRST_HALF) ? 0 : DAC_BUFFER_SIZE / 2;

  const uint16_t start_index = i; // Absolute starting index to use

  if (current_symbol_duration_us >= current_waveform_step.duration_us) { // Current sequence step has gone on long enough
    // start new symbol
    current_step++;
    updateWaveformParameters();
  }

  const uint16_t end_index = start_index + MIN(DAC_BUFFER_SIZE / 2, current_waveform_step.duration_us - current_symbol_duration_us);

  // Initial envelope modulation
  while (current_symbol_duration_us < wave_ctrl.initial_tukey_end_index && (i < end_index)) {
    uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
    uint32_t base_value = sine_table[index & (SINE_POINTS - 1)]; // Ensures nothing out of index

    uint32_t scaling_value = tukey_window[wave_ctrl.initial_tukey_window_index >> TUKEY_PRECISION] * wave_ctrl.amplitude;
    dac_buffer[i] = ((DAC_MAX_VALUE + 1) / 2) - ((scaling_value >> (4 + AMPLITUDE_PRECISION)) / 2) + (((uint64_t) base_value * scaling_value) >> (16 + AMPLITUDE_PRECISION));
    i++; 
    current_symbol_duration_us++;
    wave_ctrl.phase_accumulator += wave_ctrl.phase_increment;
    wave_ctrl.initial_tukey_window_index += wave_ctrl.tukey_increment;
  }

  // Rectangular window portion
  uint16_t offset_amt = ((DAC_MAX_VALUE + 1) / 2) - (wave_ctrl.amplitude << (12 - AMPLITUDE_PRECISION - 1));
  while (current_symbol_duration_us < wave_ctrl.final_tukey_start_index && (i < end_index)) {
    uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
    uint32_t base_value = sine_table[index & (SINE_POINTS - 1)];

    dac_buffer[i] = offset_amt + ((base_value * wave_ctrl.amplitude) >> AMPLITUDE_PRECISION);

    // Update phase
    wave_ctrl.phase_accumulator += wave_ctrl.phase_increment;
    i++;
    current_symbol_duration_us++;
  }

  // Final envelope modulation
  while (current_symbol_duration_us < current_waveform_step.duration_us && (i < end_index)) {
    uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
    uint32_t base_value = sine_table[index & (SINE_POINTS - 1)]; // Ensures nothing out of index

    uint32_t scaling_value = tukey_window[wave_ctrl.final_tukey_window_index >> TUKEY_PRECISION] * wave_ctrl.amplitude;
    dac_buffer[i] = ((DAC_MAX_VALUE + 1) / 2) - ((scaling_value >> (4 + AMPLITUDE_PRECISION)) / 2) + (((uint64_t) base_value * scaling_value) >> (16 + AMPLITUDE_PRECISION));
    i++; 
    current_symbol_duration_us++;
    wave_ctrl.phase_accumulator += wave_ctrl.phase_increment;
    wave_ctrl.final_tukey_window_index -= wave_ctrl.tukey_increment;
  }
}

/* Private function definitions ----------------------------------------------*/

// Creates a sine table with 360/SINE_POINTS degree spacing between adjacent points centered at 2047. Table has one full sine wave
void generateSineTable(void)
{
  for(uint16_t i = 0; i < SINE_POINTS; i++) {
    sine_table[i] = (uint16_t)(2047.0f * sinf(2.0f * M_PI * i / SINE_POINTS) + 2047.0f);
  }
}

void generateTukeyWindow(void)
{
  for (uint16_t i = 0; i < TUKEY_POINTS; i++) {
    tukey_window[i] = (uint16_t) (((float) USHRT_MAX / 2.0f) * (1.0f - cosf(M_PI * i / TUKEY_POINTS)));
  }
}

void updateWaveformParameters()
{
  current_waveform_step = MessDacResource_GetStep(current_step);

  // Calculate new phase increment
  wave_ctrl.phase_increment = (((uint64_t)
      current_waveform_step.freq_hz) << PHASE_PRECISION) / DAC_SAMPLE_RATE;

  wave_ctrl.amplitude = current_waveform_step.relative_amplitude * (1 << AMPLITUDE_PRECISION);

  switch (current_waveform_step.output_type) {
    case OUTPUT_CONSTANT_SQUARE:
      wave_ctrl.tukey_increment = 0;
      wave_ctrl.initial_tukey_end_index = 0;
      wave_ctrl.final_tukey_start_index = UINT_MAX;
      break;
    case OUTPUT_CONSTANT_TUKEY:
      wave_ctrl.initial_tukey_window_index = 0;
      wave_ctrl.initial_tukey_end_index = (uint32_t) (TUKEY_ALPHA * 0.5f * (float) current_waveform_step.duration_us);
      wave_ctrl.tukey_increment = (wave_ctrl.initial_tukey_end_index << TUKEY_PRECISION) / TUKEY_POINTS;
      wave_ctrl.final_tukey_window_index = (TUKEY_POINTS - 1) << TUKEY_PRECISION;
      wave_ctrl.final_tukey_start_index = current_waveform_step.duration_us - wave_ctrl.initial_tukey_end_index;
      break;
    default:
      break;
  }

  current_symbol_duration_us = 0;
}

// DMA callbacks
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dacTaskHandle, DAC_FILL_FIRST_HALF);
  }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dacTaskHandle, DAC_FILL_LAST_HALF);
  }
}

void HAL_DACEx_ConvHalfCpltCallbackCh2(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dacTaskHandle, DAC_FILL_FIRST_HALF);
  }
}

void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dacTaskHandle, DAC_FILL_LAST_HALF);
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
