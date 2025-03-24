/*
 * mess_demodulate.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_demodulate.h"
#include "mess_adc.h"
#include "mess_main.h"
#include "mess_modulate.h"

#include "cfg_defaults.h"
#include "cfg_parameters.h"

#include <stdbool.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float energy_f0;
  float energy_f1;
} DemodulationHistory_t;

/* Private define ------------------------------------------------------------*/

#define NUM_DEMODULATION_HISTORY          8 // Number of demodulations to look back on. Must be a power of 2

#define SIGNFIICANT_SHIFT_THRESHOLD       0.15

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)     ((a < b) ? (a) : (b))
#define MAX(a, b)     ((a > b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

static DemodulationDecision_t decision_method = DEFAULT_DEMOD_DECISION;
static DemodulationHistory_t demodulation_history[NUM_DEMODULATION_HISTORY][MAX_FHBFSK_NUM_TONES];

/* Private function prototypes -----------------------------------------------*/

static bool goertzel(DemodulationInfo_t* data);

/* Exported function definitions ---------------------------------------------*/

bool Demodulate_Perform(DemodulationInfo_t* data)
{
  switch (mod_demod_method) {
    case MOD_DEMOD_FSK:
      data->f0 = fsk_f0;
      data->f1 = fsk_f1;
      if (goertzel(data) == false) {
        return false;
      }
      break;
    case MOD_DEMOD_FHBFSK: {
      data->f0 = Modulate_GetFhbfskFrequency(false, data->bit_index);
      data->f1 = Modulate_GetFhbfskFrequency(true, data->bit_index);
      if (goertzel(data) == false) {
        return false;
      }
      break;
    }
    default:
      return false;
  }

  switch (decision_method) {
    case AMPLITUDE_COMPARISON:
      return true;
    /* In the HISTORICAL_COMPARISON method:
     * The algorithm uses prior demodulation results to handle cases where
     * there are significant energy shifts between f0 and f1 frequencies.
     * This helps improve bit detection reliability in multipath conditions.
     */
    case HISTORICAL_COMPARISON:
      // Expects nominally that the basic goertzel is correct, but switches predicted bit if there is a large swing
      // calculate current index

      uint16_t num_tones = (mod_demod_method == MOD_DEMOD_FSK) ? 1 : fhbfsk_num_tones;
      uint16_t frequency_index = data->bit_index % num_tones;

      uint16_t buffer_raw_length = data->bit_index / num_tones;
      uint16_t buffer_length = MIN(NUM_DEMODULATION_HISTORY, buffer_raw_length);
      uint16_t buffer_index = buffer_raw_length % NUM_DEMODULATION_HISTORY;

      // check conditions

      float delta_energy_f0, delta_energy_f1;
      float abs_normalized_delta_energy_f0, abs_normalized_delta_energy_f1;

      if (buffer_length >= 1) {
        // Look at the previous bit and check for large changes
        DemodulationHistory_t previous_result =
            demodulation_history[(buffer_index - 1) % NUM_DEMODULATION_HISTORY][frequency_index];

        delta_energy_f0 = data->energy_f0 - previous_result.energy_f0;
        delta_energy_f1 = data->energy_f1 - previous_result.energy_f1;

        abs_normalized_delta_energy_f0 = fabsf(delta_energy_f0) /
                                         MAX(data->energy_f0, data->energy_f1);

        abs_normalized_delta_energy_f1 = fabsf(delta_energy_f1) /
                                         MAX(data->energy_f0, data->energy_f1);

        float abs_normalized_delta_energy_sum = abs_normalized_delta_energy_f0 +
                                                abs_normalized_delta_energy_f1;


        bool delta_energy_f0_pos = delta_energy_f0 > 0.0f;
        bool delta_energy_f1_pos = delta_energy_f1 > 0.0f;
        bool significant_delta_energy =
            abs_normalized_delta_energy_sum > SIGNFIICANT_SHIFT_THRESHOLD;

        // significant shift towards f0
        if (delta_energy_f0_pos == true && delta_energy_f1_pos == false &&
            significant_delta_energy == true) {
          data->decoded_bit = false;
        }
        // significant shift towards f1
        if (delta_energy_f0_pos == false && delta_energy_f1_pos == true &&
            significant_delta_energy == true) {
          data->decoded_bit = true;
        }
      }

      // Add to the buffer
      demodulation_history[buffer_index][frequency_index].energy_f0 = data->energy_f0;
      demodulation_history[buffer_index][frequency_index].energy_f1 = data->energy_f1;
      break;
    default:
      return false;
  }
  return true;
}

bool Demodulate_RegisterParams()
{
  uint32_t min_u32 = MIN_DEMOD_DECISION;
  uint32_t max_u32 = MAX_DEMOD_DECISION;
  if (Param_Register(PARAM_DEMODULATION_DECISION, "the demodulation method", PARAM_TYPE_UINT8,
                     &decision_method, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }
  return true;
}

/* Private function definitions ----------------------------------------------*/

bool goertzel(DemodulationInfo_t* data)
{
  if (data == NULL) return false;

  float energy_f0 = 0.0;
  float energy_f1 = 0.0;

  float omega_f0 = 2.0 * M_PI * data->f0 / ADC_SAMPLING_RATE;
  float omega_f1 = 2.0 * M_PI * data->f1 / ADC_SAMPLING_RATE;

  float coeff_f0 = 2.0 * cosf(omega_f0);
  float coeff_f1 = 2.0 * cosf(omega_f1);

  uint16_t mask = data->buf_len - 1;

  float q0_f0 = 0, q1_f0 = 0, q2_f0 = 0;
  float q0_f1 = 0, q1_f1 = 0, q2_f1 = 0;

  for (uint16_t i = 0; i < data->data_len; i++) {
    uint16_t index = (i + data->data_start_index) & mask;

    // Foertzel algorithm for F0
    q0_f0 = coeff_f0 * q1_f0 - q2_f0 + data->data_buf[index];
    q2_f0 = q1_f0;
    q1_f0 = q0_f0;

    // Goertzel algorithm for F1
    q0_f1 = coeff_f1 * q1_f1 - q2_f1 + data->data_buf[index];
    q2_f1 = q1_f1;
    q1_f1 = q0_f1;
  }

  energy_f0 = q1_f0 * q1_f0 + q2_f0 * q2_f0 - coeff_f0 * q1_f0 * q2_f0;
  energy_f1 = q1_f1 * q1_f1 + q2_f1 * q2_f1 - coeff_f1 * q1_f1 * q2_f1;

  data->energy_f0 = energy_f0;
  data->energy_f1 = energy_f1;

  data->decoded_bit = (energy_f1 > energy_f0) ? true : false;
  data->analysis_done = true;

  return true;
}
