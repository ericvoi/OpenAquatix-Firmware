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
#include "mess_adc.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define WINDOW_PRECISION                8
#define SLIDING_CALLS_BEFORE_RESET      128

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

#define MAX_SLIDING_INDEX 32
static uint16_t sliding_goertzel_index = 0;

/* Private function prototypes -----------------------------------------------*/


/* Exported function definitions ---------------------------------------------*/

void goertzel_1(GoertzelInfo_t* goertzel_info)
{
  float energy_f0 = 0.0;

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
    float data_value = ADC_InputGetDataAbsolute(index) * window_value;

    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data_value;
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;
    window_index += window_increment;
  }

  float normalization_factor = goertzel_info->energy_normalization / goertzel_info->data_len;

  energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;

  goertzel_info->e_f[0] = energy_f0 * normalization_factor;
}

void goertzel_2(GoertzelInfo_t* goertzel_info)
{
  float energy_f0 = 0.0;
  float energy_f1 = 0.0;

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
    float data_value = ADC_InputGetDataAbsolute(index) * window_value;

    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data_value;
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;

    q0_f1 = coeff_f1 * q1_f1 - q2_f1 + data_value;
    q2_f1 = q1_f1;
    q1_f1 = q0_f1;
    window_index += window_increment;
  }

  float normalization_factor = goertzel_info->energy_normalization / goertzel_info->data_len;

  energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;
  energy_f1 = q1_f1 * q1_f1 + q2_f1 * q2_f1 - coeff_f1 * q1_f1 * q2_f1;

  goertzel_info->e_f[0] = energy_f0 * normalization_factor;
  goertzel_info->e_f[1] = energy_f1 * normalization_factor;
}

void goertzel_6(GoertzelInfo_t* goertzel_info)
{
  float energy_f0 = 0.0;
  float energy_f1 = 0.0;
  float energy_f2 = 0.0;
  float energy_f3 = 0.0;
  float energy_f4 = 0.0;
  float energy_f5 = 0.0;

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
    float data_value = ADC_InputGetDataAbsolute(index) * window_value;

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

  energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;
  energy_f1 = q1_f1 * q1_f1 + q2_f1 * q2_f1 - coeff_f0 * q1_f1 * q2_f1;
  energy_f2 = q1_f2 * q1_f2 + q2_f2 * q2_f2 - coeff_f0 * q1_f2 * q2_f2;
  energy_f3 = q1_f3 * q1_f3 + q2_f3 * q2_f3 - coeff_f0 * q1_f3 * q2_f3;
  energy_f4 = q1_f4 * q1_f4 + q2_f4 * q2_f4 - coeff_f0 * q1_f4 * q2_f4;
  energy_f5 = q1_f5 * q1_f5 + q2_f5 * q2_f5 - coeff_f0 * q1_f5 * q2_f5;

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
  goertzel_info->q[0] = 0.0f;
  goertzel_info->q[1] = 0.0f;
  goertzel_info->q[2] = 0.0f;

  float omega_f = 2.0f * goertzel_info->f / ((float) ADC_SAMPLING_RATE);
  goertzel_info->coeff = 2.0 * uam_cosf(omega_f);

  goertzel_info->normalization_factor = 1.0f / ((float) window_length);

  goertzel_info->window_length = window_length;
  goertzel_info->calls_before_reset = sliding_goertzel_index * (SLIDING_CALLS_BEFORE_RESET / MAX_SLIDING_INDEX);
  sliding_goertzel_index = (sliding_goertzel_index + 1) % MAX_SLIDING_INDEX;
}

void goertzel_SlidingPerform(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t samples, uint16_t buf_len)
{
  uint16_t mask = buf_len - 1;

  if (goertzel_info->calls_before_reset == 0) {
    start_index = start_index + samples - goertzel_info->window_length;
    start_index &= mask;
    goertzel_info->q[0] = 0.0f;
    goertzel_info->q[1] = 0.0f;
    goertzel_info->q[2] = 0.0f;

    goertzel_info->calls_before_reset = SLIDING_CALLS_BEFORE_RESET;
    goertzel_SlidingReset(goertzel_info, start_index, buf_len);
    return;
  }
  goertzel_info->calls_before_reset--;

  for (uint16_t i = 0; i < samples; i++) {
    uint16_t new_index = (i + start_index) & mask;
    uint16_t old_index = (new_index - goertzel_info->window_length) & mask;
    float new_data_value = ADC_InputGetDataAbsolute(new_index);
    float old_data_value = ADC_InputGetDataAbsolute(old_index);

    goertzel_info->q[0] = goertzel_info->coeff * goertzel_info->q[1] - goertzel_info->q[2] + new_data_value - old_data_value;
    goertzel_info->q[2] = goertzel_info->q[1];
    goertzel_info->q[1] = goertzel_info->q[0];
  }

  goertzel_info->e_f = goertzel_info->q[1] * goertzel_info->q[1] + 
                       goertzel_info->q[2] * goertzel_info->q[2] -
                       goertzel_info->coeff * goertzel_info->q[1] * goertzel_info->q[2];

  goertzel_info->e_f *= goertzel_info->normalization_factor;
}

void goertzel_SlidingReset(SlidingGoertzelInfo_t* goertzel_info, uint16_t start_index, uint16_t buf_len)
{
  uint16_t mask = buf_len - 1;
  for (uint16_t i = 0; i < goertzel_info->window_length; i++) {
    uint16_t index = (i + start_index) & mask;
    float data_value = ADC_InputGetDataAbsolute(index);

    goertzel_info->q[0] = goertzel_info->coeff * goertzel_info->q[1] - goertzel_info->q[2] + data_value;
    goertzel_info->q[2] = goertzel_info->q[1];
    goertzel_info->q[1] = goertzel_info->q[0];
  }

  goertzel_info->e_f = goertzel_info->q[1] * goertzel_info->q[1] + 
                       goertzel_info->q[2] * goertzel_info->q[2] -
                       goertzel_info->coeff * goertzel_info->q[1] * goertzel_info->q[2];

  goertzel_info->e_f *= goertzel_info->normalization_factor;
}

/* Private function definitions ----------------------------------------------*/
