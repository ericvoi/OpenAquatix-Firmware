/*
 * dac_waveform.c
 *
 *  Created on: Feb 5, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "dac_waveform.h"
#include "dac_main.h"
#include "mess_filt_resources.h"
#include "mess_modulate.h"
#include "mess_dac_resources.h"
#include "cfg_defaults.h"
#include "cfg_parameters.h"
#include "sleep/wakeup_tones.h"
#include "mess_ranging.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "error_manager.h"
#include "core_cm7.h"
#include "hil_manager.h"
#include "hil_main.h"
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint32_t pos;
  union {
    struct {uint32_t phase, phase_increment;} nco;
    struct {uint32_t phase, increment; int32_t increment_delta;} lfm;
    struct {uint32_t phase; float pi0, g, dg;} hfm;
  } k;
} ModState_t;

/* Private define ------------------------------------------------------------*/

#define SINE_LUT_POWER      10
#define SINE_POINTS         (1 << SINE_LUT_POWER)
#define DAC_MAX_VALUE       4095
#define PHASE_PRECISION     32
#define PHASE_SHIFT         (PHASE_PRECISION - SINE_LUT_POWER)

#define TUKEY_POINTS        (1 << 8) // 256 points in pre-computed Tukey array

#define TUKEY_ALPHA_NCO     (0.05f)
#define TUKEY_ALPHA_CHIRP   (0.02f)

#define SAFETY_MAX          (3.0f / 3.3f) // Protects the PA against overvoltage
#define DAC_MID             ((DAC_MAX_VALUE + 1) / 2)

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)          ((a < b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

extern osThreadId_t dac_taskHandle;

static float sine_table[SINE_POINTS] __attribute__((section(".dtcm")));
static uint16_t dac_buffer[DAC_BUFFER_SIZE] __attribute__((section(".dma_buf")));

static float scratch_buf[DAC_BUFFER_SIZE / 2];
static ModState_t modulation_state __attribute__((section(".dtcm")));
static Symbol_t curr_symbol;
static volatile uint32_t sequence_length = 0;
static volatile uint16_t current_step = 0;
static volatile bool dac_running = false;

// Flag that indicates that the next time this function is called it should terminate the DAC output
static bool last_fill = false;

static bool delay_next_message = false;
static uint32_t delay_cyccnt = 0;

static bool apply_tukey = DEFAULT_APPLY_TUKEY;

static volatile uint32_t callback_count = 0;

static float tukey_table[TUKEY_POINTS] __attribute__((section(".dtcm")));

extern osThreadId_t hil_taskHandle;

// Output tone that flushes out the DAC and prevents the first message from being scrambled
Symbol_t test_step = {
  .output_type = OUTPUT_NCO,
  .ramp_samples = 0,
  .relative_amplitude = 0.0f,
  .n_samples = 1000000 / DAC_SUBSAMPLING, // Any lower duration does not work
  .u.nco = {.freq_hz = 30000},
};

/* Private function prototypes -----------------------------------------------*/

static void generateSineTable(void);
static void generateTukeyWindow(void);
static void updateWaveformParameters(void);

static void fillBufferNco(float* buf, uint16_t n, ModState_t* state);
static void fillBufferLfm(float* buf, uint16_t n, ModState_t* state);
static void fillBufferHfm(float* buf, uint16_t n, ModState_t* state);

static void applyEnvelope(uint16_t* dst, const float* src, uint16_t n,
                          const Symbol_t* step, const ModState_t* state);

static inline float sineLut(uint32_t phase) {
  return sine_table[(phase >> PHASE_SHIFT) & (SINE_POINTS - 1)];
}
static inline float tukeyLut(uint32_t index, uint32_t ramp_duration) {
  return tukey_table[index * TUKEY_POINTS / ramp_duration];
}

static inline uint32_t phaseInc(uint32_t freq) {
  return (((uint64_t) freq) << PHASE_PRECISION) / DAC_SAMPLE_RATE; 
} 

/* Exported function definitions ---------------------------------------------*/

void Waveform_InitWaveformGenerator(void)
{
  generateSineTable();
  generateTukeyWindow();

  // Initialize control structure
  memset(&modulation_state, 0, sizeof(modulation_state));
  callback_count = 0;
  current_step = 0;
  dac_running = false;
  sequence_length = 0;

  // Configure DAC and DMA here
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
}

bool Waveform_SetWaveformSequence(uint16_t num_steps, bool is_message, bool delay, uint32_t cyccnt)
{
  if (num_steps == 0) return false;
  if (dac_running == true) return false;

  if (delay == true) {
    delay_next_message = true;
    delay_cyccnt = cyccnt;
  }

  uint16_t extra_steps = is_message ? MessDacResource_SyncWakeupSteps() : 0;

  sequence_length = num_steps + extra_steps;
  current_step = 0;

  return true;
}

bool Waveform_PrepareWaveformOutput(uint32_t channel)
{
  if (HAL_DAC_Stop_DMA(&hdac1, channel) != HAL_OK) return false;

  last_fill = false;
  dac_running = true;
  updateWaveformParameters();
  Waveform_FillBuffer(FILL_FIRST_HALF);
  Waveform_FillBuffer(FILL_LAST_HALF);

  HAL_StatusTypeDef ret = HAL_DAC_Start_DMA(&hdac1, channel, (uint32_t*) dac_buffer,
                    DAC_BUFFER_SIZE, DAC_ALIGN_12B_R);

  return ret == HAL_OK;
}

void Waveform_StartOutput(void)
{
  if (delay_next_message == false) {
    HAL_TIM_Base_Start(&htim6);
    return;
  }
  delay_next_message = false;

  uint32_t current_cyccnt = DWT->CYCCNT;
  uint32_t ms_to_wait = (delay_cyccnt - current_cyccnt) / (SystemCoreClock / 1000);
  if (ms_to_wait > 6) ms_to_wait -= 3; // Wait 3ms for CYCCNT condition
  osDelay(ms_to_wait);
  uint32_t start_difference = delay_cyccnt - DWT->CYCCNT;
  while (start_difference > (delay_cyccnt - DWT->CYCCNT)) { ;}
  HAL_TIM_Base_Start(&htim6);
}

void Waveform_SendRangingRequest(void)
{
  portENTER_CRITICAL();
  Ranging_LogRequest();
  HAL_TIM_Base_Start(&htim6);
  portEXIT_CRITICAL();
}

bool Waveform_StopWaveformOutput()
{
  // reset flags and end DMA transfer to ease DMA channels
  dac_running = false;
  osEventFlagsSet(print_event_handle, MESS_DAC_MESS_DONE);
  HAL_TIM_Base_Stop(&htim6);
  if (HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1) != HAL_OK) return false;
  return true;
}

bool Waveform_IsRunning()
{
  return dac_running;
}

void Waveform_RegisterParams()
{
  uint32_t min_u32 = MIN_APPLY_TUKEY;
  uint32_t max_u32 = MAX_APPLY_TUKEY;
  if (Param_Register(PARAM_APPLY_TUKEY, "Tukey window modulation", PARAM_TYPE_UINT8,
                     &apply_tukey, sizeof(bool), &min_u32, &max_u32, NULL, NULL) == false) {
    REGISTER_ERROR(ERROR_PARAMETER_REGISTRATION);
  }
}

void Waveform_Flush()
{
  RETURN_IF_ERROR_PRESENT();
  if (Waveform_SetWaveformSequence(1, false, false, 0) == false)
    REGISTER_ERROR(ERROR_DAC_FLUSH);
  if (HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1) != HAL_OK)
    REGISTER_ERROR(ERROR_DAC_FLUSH);

  dac_running = true;
  modulation_state.k.nco.phase = 0;
  modulation_state.k.nco.phase_increment = 0;
  modulation_state.pos = 0;

  memcpy(&curr_symbol, &test_step, sizeof(Symbol_t));
  Waveform_FillBuffer(FILL_FIRST_HALF);
  Waveform_FillBuffer(FILL_LAST_HALF);

  if (HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*) dac_buffer,
      DAC_BUFFER_SIZE, DAC_ALIGN_12B_R) != HAL_OK)
    REGISTER_ERROR(ERROR_DAC_FLUSH);

  if (HAL_TIM_Base_Start(&htim6) != HAL_OK)
    REGISTER_ERROR(ERROR_DAC_FLUSH);

  while (dac_running == true)
    osDelay(1);
  
  if (HAL_TIM_Base_Stop(&htim6) != HAL_OK)
    REGISTER_ERROR(ERROR_DAC_FLUSH);
}

void Waveform_FillBuffer(FillType_t type)
{
  callback_count++;

  if (last_fill) {
    last_fill = false;
    Waveform_StopWaveformOutput();
    return;
  }

  uint16_t i = (type == FILL_FIRST_HALF) ? 0 : DAC_BUFFER_SIZE / 2;
  const uint16_t end = i + DAC_BUFFER_SIZE / 2;

  while (i < end) {

    if (modulation_state.pos >= curr_symbol.n_samples) {
      if (current_step >= sequence_length - 1) {
        // Last symbol is done — fill remainder with DC midpoint so the
        // DMA never outputs stale data, then schedule a stop
        while (i < end)
          dac_buffer[i++] = (DAC_MAX_VALUE + 1) / 2;
        last_fill = true;
        return;
      }
      current_step++;
      updateWaveformParameters();
    }

    uint16_t n = MIN((uint32_t) end - i, curr_symbol.n_samples - modulation_state.pos);
    if (n == 0) REGISTER_ERROR(ERROR_INVALID_DAC_SEQ_LEN);

    switch (curr_symbol.output_type) {
      case OUTPUT_NCO:
        fillBufferNco(scratch_buf, n, &modulation_state);
        break;
      case OUTPUT_LFM:
        fillBufferLfm(scratch_buf, n, &modulation_state);
        break;
      case OUTPUT_HFM:
        fillBufferHfm(scratch_buf, n, &modulation_state);
      default:
        REGISTER_ERROR(ERROR_UNHANDLED_CASE);
        break;
    }
    applyEnvelope(&dac_buffer[i], scratch_buf, n, &curr_symbol, &modulation_state);
    i += n;
    modulation_state.pos += n;
  } // while (i < half_end)
}

/* Private function definitions ----------------------------------------------*/

// Creates a sine table with 360/SINE_POINTS degree spacing between adjacent points centered at 2047. Table has one full sine wave
void generateSineTable(void)
{
  for(uint16_t i = 0; i < SINE_POINTS; i++) {
    sine_table[i] = sinf(2.0f * M_PI * i / SINE_POINTS);
  }
}

// Minor bug: the last index is never reached in the tukey window due to integer truncation
void generateTukeyWindow(void)
{
  for (uint16_t i = 0; i < TUKEY_POINTS; i++) {
    tukey_table[i] = (1.0f - cosf(M_PI * i / TUKEY_POINTS)) * 0.5f;
  }
}

void updateWaveformParameters()
{
  modulation_state.pos = 0;
  curr_symbol = MessDacResource_GetStep(current_step);

  curr_symbol.n_samples = ((uint64_t) curr_symbol.duration_ns * DAC_SAMPLE_RATE) /
                                    1E9;

  switch (curr_symbol.output_type) {
    case OUTPUT_NCO:
      modulation_state.k.nco.phase_increment = phaseInc(curr_symbol.u.nco.freq_hz);
      break;
    case OUTPUT_LFM: {
      modulation_state.k.lfm.phase = 0;
      int32_t inc_start = (int32_t) phaseInc(curr_symbol.u.chirp.f_start_hz);
      int32_t inc_end   = (int32_t) phaseInc(curr_symbol.u.chirp.f_end_hz);
      modulation_state.k.lfm.increment = inc_start;
      modulation_state.k.lfm.increment_delta = (curr_symbol.n_samples > 0)
          ? ((inc_end - inc_start) / (int32_t) curr_symbol.n_samples)
          : 0;
      break;
    }
    case OUTPUT_HFM: {
      float f0 = curr_symbol.u.chirp.f_start_hz;
      float f1 = curr_symbol.u.chirp.f_end_hz;
      modulation_state.k.hfm.pi0 = (float) phaseInc(curr_symbol.u.chirp.f_start_hz);
      modulation_state.k.hfm.g = 1.0f;
      modulation_state.k.hfm.dg = (f0 / f1 - 1.0f) / curr_symbol.n_samples;
      break;
    }
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
      break;
  }

  float tukey_alpha = 0.0f;
  switch (curr_symbol.output_type) {
    case OUTPUT_NCO:
      tukey_alpha = TUKEY_ALPHA_NCO;
      break;
    case OUTPUT_LFM:
    case OUTPUT_HFM:
      tukey_alpha = TUKEY_ALPHA_CHIRP;
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
      break;
  }

  if (apply_tukey == false) tukey_alpha = 0.0f;

  curr_symbol.ramp_samples =
      (uint32_t) (curr_symbol.n_samples * tukey_alpha / 2.0f);
}

static void fillBufferNco(float* buf, uint16_t n, ModState_t* state)
{
  uint32_t phase = state->k.nco.phase;
  const uint32_t phase_increment = state->k.nco.phase_increment;
  for (uint16_t i = 0; i < n; i++) {
    buf[i] = sineLut(phase);
    phase += phase_increment;
  }
  state->k.nco.phase = phase;
}

static void fillBufferLfm(float* buf, uint16_t n, ModState_t* state)
{
  uint32_t phase = state->k.lfm.phase;
  uint32_t increment = state->k.lfm.increment;
  const uint32_t increment_delta = state->k.lfm.increment_delta;
  for (uint16_t i = 0; i < n; i++) {
    buf[i] = sineLut(phase);
    phase += increment;
    increment += increment_delta;
  }
  state->k.lfm.phase = phase;
  state->k.lfm.increment = increment;
}

static void fillBufferHfm(float* buf, uint16_t n, ModState_t* state)
{
  uint32_t phase = state->k.hfm.phase;
  float g = state->k.hfm.g;
  const float dg = state->k.hfm.dg;
  const float pi0 = state->k.hfm.pi0;
  for (uint16_t i = 0; i < n; i++) {
    buf[i] = sineLut(phase);
    phase += (uint32_t)(pi0 / g + 0.5f);
    g += dg;
  }
  state->k.hfm.phase = phase;
  state->k.hfm.g = g;
}

static void applyEnvelope(uint16_t* dst, const float* src, uint16_t n,
                          const Symbol_t* step, const ModState_t* state)
{
  for (uint16_t i = 0; i < n; i++) {
    // Get the envelope attenuation
    uint32_t p = state->pos + i;
    float w = 1.0f;
    if (step->ramp_samples != 0) {
      if (p < step->ramp_samples) 
        w = tukeyLut(p, step->ramp_samples);
      else if (p >= step->n_samples - step->ramp_samples)
        w = tukeyLut(step->n_samples - 1 - p, step->ramp_samples);
    }
    // Apply amplitude scale and window
    float scaled = w * step->relative_amplitude * src[i];
    // Clamp to protect PA against damage
    if (scaled >  SAFETY_MAX) scaled =  SAFETY_MAX;
    if (scaled < -SAFETY_MAX) scaled = -SAFETY_MAX;

    dst[i] = (uint16_t)(DAC_MID + scaled * DAC_MID);
  }
}

// DMA callbacks
void HAL_DAC_ConvHalfCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dac_taskHandle, DAC_FILL_FIRST_HALF);
    return;
  }
  if (HilManager_HilMode() == HIL_STATE_RX) {
    osThreadFlagsSet(hil_taskHandle, HIL_EVT_DAC_HALF_FULL);
    return;
  }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
  (void)(hdac);
  if (dac_running == true) {
    osThreadFlagsSet(dac_taskHandle, DAC_FILL_LAST_HALF);
    return;
  }
  if (HilManager_HilMode() == HIL_STATE_RX) {
    osThreadFlagsSet(hil_taskHandle, HIL_EVT_DAC_FULL);
    return;
  }
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
  (void)(hdma);
    // Break here or log error
}
