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
#include "FreeRTOS.h"
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

/* Private define ------------------------------------------------------------*/

#define SINE_POINTS         1024
#define DAC_MAX_VALUE       4095
#define PHASE_PRECISION     32

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern osThreadId_t dacTaskHandle;

static uint16_t sine_table[SINE_POINTS];
static uint32_t dac_buffer[DAC_BUFFER_SIZE] = {0};

static WaveformControl_t wave_ctrl;
static WaveformStep_t current_waveform_step;
static volatile uint32_t sequence_length = 0;
static volatile uint16_t current_step = 0;
static volatile bool dac_running = false;
static uint32_t current_symbol_duration_us = 0;

static volatile uint32_t callback_count = 0;

static uint16_t transition_length = DEFAULT_DAC_TRANSITION_LEN;

// Output tone that flushes out the DAC and prevents the first message from being scrambled
WaveformStep_t test_step = {
    .duration_us = 1000000, // Any lower duration does not work
    .freq_hz = 30000,
    .relative_amplitude = 0.0
};

/* Private function prototypes -----------------------------------------------*/

static void generateSineTable(void);
static void updateWaveformParameters(void);

/* Exported function definitions ---------------------------------------------*/

bool Waveform_InitWaveformGenerator(void)
{
  generateSineTable();

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

bool Waveform_SetWaveformSequence(uint16_t num_steps)
{
  if(num_steps == 0) return false;

  sequence_length = num_steps;
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
  wave_ctrl.current_amplitude = 0;
  wave_ctrl.target_amplitude = 0;
  wave_ctrl.amplitude_transitioning = false;

  ADC_StopFeedback();
  return true;
}

bool Waveform_IsRunning()
{
  return dac_running;
}

bool Waveform_RegisterParams()
{
  uint32_t min = MIN_DAC_TRANSITION_LEN;
  uint32_t max = MAX_DAC_TRANSITION_LEN;
  if (Param_Register(PARAM_DAC_TRANSITION_LEN, "DAC transition duration (us)", PARAM_TYPE_UINT16,
                     &transition_length, sizeof(uint16_t), &min, &max) == false) {
    return false;
  }

  return true;
}

void Waveform_Flush()
{
  Waveform_SetWaveformSequence(1);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_FEEDBACK);
  wave_ctrl.phase_accumulator = 0;

  dac_running = true;
  wave_ctrl.phase_increment = 0;

  wave_ctrl.target_amplitude = test_step.relative_amplitude;
  wave_ctrl.amplitude_step = 0;
  wave_ctrl.amplitude_counter = 0;
  wave_ctrl.amplitude_transitioning = false;

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
  const uint16_t end_index = (type == FILL_FIRST_HALF) ? DAC_BUFFER_SIZE / 2: DAC_BUFFER_SIZE;

  if (current_symbol_duration_us >= current_waveform_step.duration_us) { // Current sequence step has gone on long enough
    // start new symbol
    current_step++;
    updateWaveformParameters();
  }

  // Flag to change the output frequency has been set so perform amplitude transition
  if (wave_ctrl.amplitude_transitioning) {
    for (;i < start_index + transition_length; i++) {
      // Take the first 10 bits of the phase as the sine table has 2^10 points
      uint32_t index = wave_ctrl.phase_accumulator >> (PHASE_PRECISION - 10);
      uint32_t base_value = sine_table[index & (SINE_POINTS - 1)]; // Ensures nothing out of index

      wave_ctrl.current_amplitude = (uint32_t) ((int32_t) wave_ctrl.current_amplitude + wave_ctrl.amplitude_step);
      wave_ctrl.amplitude_counter++;
      if (wave_ctrl.amplitude_counter >= transition_length) {
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
  for (; i < end_index; i++) {
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

/* Private function definitions ----------------------------------------------*/

// Creates a sine table with 360/SINE_POINTS degree spacing between adjacent points centered at 2047. Table has one full sine wave
static void generateSineTable(void)
{
  for(uint16_t i = 0; i < SINE_POINTS; i++) {
    sine_table[i] = (uint16_t)(2047.0f * sinf(2.0f * M_PI * i / SINE_POINTS) + 2047.0f);
  }
}

static void updateWaveformParameters()
{
  current_waveform_step = MessDacResource_GetStep(current_step);

  // Calculate new phase increment
  wave_ctrl.phase_increment = (((uint64_t)
      current_waveform_step.freq_hz) << PHASE_PRECISION) / DAC_SAMPLE_RATE;

  // Setup amplitude transition
  wave_ctrl.target_amplitude = (uint32_t)
      (current_waveform_step.relative_amplitude * (float) DAC_MAX_VALUE);
  wave_ctrl.amplitude_step = ((int32_t) wave_ctrl.target_amplitude - (int32_t) wave_ctrl.current_amplitude) / transition_length;
  wave_ctrl.amplitude_counter = 0;
  wave_ctrl.amplitude_transitioning = true;

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
