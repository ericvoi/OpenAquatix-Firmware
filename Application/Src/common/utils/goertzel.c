/*
 * goertzel.c
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "goertzel.h"
#include "uam_math.h"
#include "mess_adc.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define WINDOW_PRECISION    8

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



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

/* Private function definitions ----------------------------------------------*/
