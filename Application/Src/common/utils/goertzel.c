/*
 * goertzel.c
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "goertzel.h"
#include "uam_math.h"
#include "mess_filt_resources.h"
#include "math.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define WINDOW_PRECISION                8
#define SLIDING_CALLS_BEFORE_RESET      64

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

#define MAX_SLIDING_INDEX 32
static uint16_t sliding_goertzel_index = 0;

/* Private function prototypes -----------------------------------------------*/


/* Exported function definitions ---------------------------------------------*/

void goertzel_1(GoertzelInfo_t* goertzel_info)
{
  float omega_f0 = 2.0 * goertzel_info->f[0] / ADC_SAMPLING_RATE;

  float coeff_f0 = 2.0 * uam_cosf(omega_f0);

  uint16_t mask = goertzel_info->buf_len - 1;

  float q0_f0 = 0, q1_f0 = 0, q2_f0 = 0;

  uint32_t window_index = 0;
  uint32_t window_increment = (goertzel_info->window_size << WINDOW_PRECISION)
                              / goertzel_info->data_len;

  for (uint16_t i = 0; i < goertzel_info->data_len; i++) {
    float window_value = goertzel_info->window[(window_index >> WINDOW_PRECISION)];
    uint16_t index = (i + goertzel_info->start_pos) & mask;
    float data_value = MessFiltResources_GetInputDataAbsolute(index) * window_value;

    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data_value;
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;
    window_index += window_increment;
  }

  float normalization_factor = goertzel_info->energy_normalization / goertzel_info->data_len;

  float energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;

  goertzel_info->e_f[0] = energy_f0 * normalization_factor;
}

// Unrolled to improve performance by unrolling loops and decreasing windowing
// operations required. Uses 10/32 floating-point registers in the main loop
void goertzel_2(GoertzelInfo_t* goertzel_info)
{
  float omega_f0 = 2.0 * goertzel_info->f[0] / ADC_SAMPLING_RATE;
  float omega_f1 = 2.0 * goertzel_info->f[1] / ADC_SAMPLING_RATE;

  float coeff_f0 = 2.0 * uam_cosf(omega_f0);
  float coeff_f1 = 2.0 * uam_cosf(omega_f1);

  uint16_t mask = goertzel_info->buf_len - 1;

  float q0_f0 = 0, q1_f0 = 0, q2_f0 = 0;
  float q0_f1 = 0, q1_f1 = 0, q2_f1 = 0;

  uint32_t window_index = 0;
  uint32_t window_increment = (goertzel_info->window_size << WINDOW_PRECISION)
                              / goertzel_info->data_len;

  for (uint16_t i = 0; i < goertzel_info->data_len; i++) {
    float window_value = goertzel_info->window[(window_index >> WINDOW_PRECISION)];
    uint16_t index = (i + goertzel_info->start_pos) & mask;
    float data_value = MessFiltResources_GetInputDataAbsolute(index) * window_value;

    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data_value;
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;

    q0_f1 = coeff_f1 * q1_f1 - q2_f1 + data_value;
    q2_f1 = q1_f1;
    q1_f1 = q0_f1;
    window_index += window_increment;
  }

  float normalization_factor = goertzel_info->energy_normalization / goertzel_info->data_len;

  float energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;
  float energy_f1 = q1_f1 * q1_f1 + q2_f1 * q2_f1 - coeff_f1 * q1_f1 * q2_f1;

  goertzel_info->e_f[0] = energy_f0 * normalization_factor;
  goertzel_info->e_f[1] = energy_f1 * normalization_factor;
}

// Unrolled to improve performance by unrolling loops and decreasing windowing
// operations required. Uses 26/32 floating-point registers in the main loop
void goertzel_6(GoertzelInfo_t* goertzel_info)
{
  float omega_f0 = 2.0 * goertzel_info->f[0] / ADC_SAMPLING_RATE;
  float omega_f1 = 2.0 * goertzel_info->f[1] / ADC_SAMPLING_RATE;
  float omega_f2 = 2.0 * goertzel_info->f[2] / ADC_SAMPLING_RATE;
  float omega_f3 = 2.0 * goertzel_info->f[3] / ADC_SAMPLING_RATE;
  float omega_f4 = 2.0 * goertzel_info->f[4] / ADC_SAMPLING_RATE;
  float omega_f5 = 2.0 * goertzel_info->f[5] / ADC_SAMPLING_RATE;

  float coeff_f0 = 2.0 * uam_cosf(omega_f0);
  float coeff_f1 = 2.0 * uam_cosf(omega_f1);
  float coeff_f2 = 2.0 * uam_cosf(omega_f2);
  float coeff_f3 = 2.0 * uam_cosf(omega_f3);
  float coeff_f4 = 2.0 * uam_cosf(omega_f4);
  float coeff_f5 = 2.0 * uam_cosf(omega_f5);

  uint16_t mask = goertzel_info->buf_len - 1;

  float q0_f0 = 0, q1_f0 = 0, q2_f0 = 0;
  float q0_f1 = 0, q1_f1 = 0, q2_f1 = 0;
  float q0_f2 = 0, q1_f2 = 0, q2_f2 = 0;
  float q0_f3 = 0, q1_f3 = 0, q2_f3 = 0;
  float q0_f4 = 0, q1_f4 = 0, q2_f4 = 0;
  float q0_f5 = 0, q1_f5 = 0, q2_f5 = 0;

  uint32_t window_index = 0;
  uint32_t window_increment = (goertzel_info->window_size << WINDOW_PRECISION)
                              / goertzel_info->data_len;

  for (uint16_t i = 0; i < goertzel_info->data_len; i++) {
    float window_value = goertzel_info->window[(window_index >> WINDOW_PRECISION)];
    uint16_t index = (i + goertzel_info->start_pos) & mask;
    float data_value = MessFiltResources_GetInputDataAbsolute(index) * window_value;

    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data_value;
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;

    q0_f1 = coeff_f1 * q1_f1 - q2_f1 + data_value;
    q2_f1 = q1_f1;
    q1_f1 = q0_f1;

    q0_f2 = coeff_f2 * q1_f2 - q2_f2 + data_value;
    q2_f2 = q1_f2;
    q1_f2 = q0_f2;

    q0_f3 = coeff_f3 * q1_f3 - q2_f3 + data_value;
    q2_f3 = q1_f3;
    q1_f3 = q0_f3;

    q0_f4 = coeff_f4 * q1_f4 - q2_f4 + data_value;
    q2_f4 = q1_f4;
    q1_f4 = q0_f4;

    q0_f5 = coeff_f5 * q1_f5 - q2_f5 + data_value;
    q2_f5 = q1_f5;
    q1_f5 = q0_f5;
    window_index += window_increment;
  }

  float normalization_factor = goertzel_info->energy_normalization / goertzel_info->data_len;

  float energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;
  float energy_f1 = q1_f1 * q1_f1 + q2_f1 * q2_f1 - coeff_f0 * q1_f1 * q2_f1;
  float energy_f2 = q1_f2 * q1_f2 + q2_f2 * q2_f2 - coeff_f0 * q1_f2 * q2_f2;
  float energy_f3 = q1_f3 * q1_f3 + q2_f3 * q2_f3 - coeff_f0 * q1_f3 * q2_f3;
  float energy_f4 = q1_f4 * q1_f4 + q2_f4 * q2_f4 - coeff_f0 * q1_f4 * q2_f4;
  float energy_f5 = q1_f5 * q1_f5 + q2_f5 * q2_f5 - coeff_f0 * q1_f5 * q2_f5;

  goertzel_info->e_f[0] = energy_f0 * normalization_factor;
  goertzel_info->e_f[1] = energy_f1 * normalization_factor;
  goertzel_info->e_f[2] = energy_f2 * normalization_factor;
  goertzel_info->e_f[3] = energy_f3 * normalization_factor;
  goertzel_info->e_f[4] = energy_f4 * normalization_factor;
  goertzel_info->e_f[5] = energy_f5 * normalization_factor;
}

void goertzel_SlidingInit(SlidingGoertzelInfo_t* goertzel_info, uint32_t f, uint16_t window_length)
{
  goertzel_info->f = f;
  goertzel_info->x_real = 0.0f;
  goertzel_info->x_imag = 0.0f;

  float omega_normalized = 2.0f * M_PI * (float)f / (float)ADC_SAMPLING_RATE;
  
  // No CORDIC acceleration since high accuracy required for stability
  goertzel_info->cos_omega = cosf(omega_normalized);
  goertzel_info->sin_omega = sinf(omega_normalized);
  goertzel_info->coeff = 2.0f * goertzel_info->cos_omega;

  goertzel_info->normalization_factor = 1.0f / (float)window_length;

  goertzel_info->window_length = window_length;
  
  // Stagger resets across filters to spread computational load
  goertzel_info->calls_before_reset = sliding_goertzel_index * SLIDING_CALLS_BEFORE_RESET / MAX_SLIDING_INDEX;
  sliding_goertzel_index = (sliding_goertzel_index + 1) % MAX_SLIDING_INDEX;
}

void goertzel_SlidingPerform(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t samples, uint16_t buf_len)
{
  uint16_t mask = buf_len - 1;

  // Periodic reset to combat numerical drift
  if (goertzel_info->calls_before_reset == 0) {
    uint16_t window_start = (start_index + samples - goertzel_info->window_length) & mask;
    
    goertzel_info->calls_before_reset = SLIDING_CALLS_BEFORE_RESET;
    goertzel_SlidingReset(goertzel_info, window_start, buf_len);
    return;
  }
  goertzel_info->calls_before_reset--;

  const float cos_w = goertzel_info->cos_omega;
  const float sin_w = goertzel_info->sin_omega;
  const uint16_t win_len = goertzel_info->window_length;
  
  float x_real = goertzel_info->x_real;
  float x_imag = goertzel_info->x_imag;

  // Formula: X_new = e^(jω) × (X_old + x_new - x_old)
  for (uint16_t i = 0; i < samples; i++) {
    uint16_t new_index = (start_index + i) & mask;
    uint16_t old_index = (new_index - win_len) & mask;
    
    float new_sample = MessFiltResources_GetInputDataAbsolute(new_index);
    float old_sample = MessFiltResources_GetInputDataAbsolute(old_index);

    float delta = new_sample - old_sample;
    float temp_real = x_real + delta;
    float temp_imag = x_imag;

    // This rotation accounts for all samples shifting one position in the window,
    // which changes their phase contribution to the DFT
    x_real = temp_real * cos_w - temp_imag * sin_w;
    x_imag = temp_real * sin_w + temp_imag * cos_w;
  }

  // Store updated state
  goertzel_info->x_real = x_real;
  goertzel_info->x_imag = x_imag;

  // Compute energy: |X|² = Re(X)² + Im(X)²
  goertzel_info->e_f = (x_real * x_real + x_imag * x_imag) * goertzel_info->normalization_factor;
}

void goertzel_SlidingReset(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t buf_len)
{
  uint16_t mask = buf_len - 1;
  
  float q0 = 0.0f;
  float q1 = 0.0f;
  float q2 = 0.0f;
  const float coeff = goertzel_info->coeff;
  
  for (uint16_t i = 0; i < goertzel_info->window_length; i++) {
    uint16_t index = (start_index + i) & mask;
    float sample = MessFiltResources_GetInputDataAbsolute(index);

    q0 = sample + coeff * q1 - q2;
    q2 = q1;
    q1 = q0;
  }

  // X[k] = s[N-1] - e^(-jω)·s[N-2]
  //      = s[N-1] - (cos(ω) - j·sin(ω))·s[N-2]
  //      = (s[N-1] - cos(ω)·s[N-2]) + j·(sin(ω)·s[N-2])
  goertzel_info->x_real = q1 - goertzel_info->cos_omega * q2;
  goertzel_info->x_imag = goertzel_info->sin_omega * q2;

  float x_real = goertzel_info->x_real;
  float x_imag = goertzel_info->x_imag;
  goertzel_info->e_f = (x_real * x_real + x_imag * x_imag) * goertzel_info->normalization_factor;
}

/* Private function definitions ----------------------------------------------*/
