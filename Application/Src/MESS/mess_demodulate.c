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

#include "uam_math.h"
#include "goertzel.h"

#include <stdbool.h>
#include <math.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float energy_f0;
  float energy_f1;
} DemodulationHistory_t;

/* Private define ------------------------------------------------------------*/

#define NUM_DEMODULATION_HISTORY          8 // Number of demodulations to look back on. Must be a power of 2

#define OVERWHELMING_ENERGY_THRESHOLD     (6.25f) // If the energy ratio is greater than this value then dont do historical comparison

#define WINDOW_FUNCTION_SIZE              (1 << 9) // 512
#define WINDOW_INCREMENT_PRECISION        (9)

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)     ((a < b) ? (a) : (b))
#define MAX(a, b)     ((a > b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

static DemodulationDecision_t decision_method = DEFAULT_DEMOD_DECISION;

static uint32_t lower_calibration_frequency = DEFAULT_DEMOD_CAL_LOWER_F;
static uint32_t upper_calibration_frequency = DEFAULT_DEMOD_CAL_UPPER_F;

static DemodulationHistory_t demodulation_history[NUM_DEMODULATION_HISTORY][MAX_FHBFSK_NUM_TONES];

static float significant_shift_threshold = DEFAULT_HIST_CMP_THRESH;

static WindowFunction_t window_function = DEFAULT_WINDOW_FUNCTION;
float window[WINDOW_FUNCTION_SIZE];

/* Private function prototypes -----------------------------------------------*/

static void GoertzelInfoCopy(GoertzelInfo_t* goertzel_info, DemodulationInfo_t* data);
static void updateWindow();
static void setWindowRectangular();
static void setWindowHann();
static void setWindowHamming();

/* Exported function definitions ---------------------------------------------*/

void Demodulate_Init()
{
  updateWindow();
}

bool Demodulate_Perform(DemodulationInfo_t* data, const DspConfig_t* cfg)
{
  switch (cfg->mod_demod_method) {
    case MOD_DEMOD_FSK:
      GoertzelInfo_t goertzel_info;
      GoertzelInfoCopy(&goertzel_info, data);
      uint32_t f[2] = {cfg->fsk_f0, cfg->fsk_f1};
      float e_f[2];
      goertzel_info.f = f;
      goertzel_info.e_f = e_f;
      goertzel_info.energy_normalization = Demodulate_PowerNormalization();

      goertzel_2(&goertzel_info);

      data->analysis_done = true;
      data->decoded_bit = (goertzel_info.e_f[0] > goertzel_info.e_f[1]) ? false : true;
      break;
    case MOD_DEMOD_FHBFSK: {
      data->f0 = Modulate_GetFhbfskFrequency(false, data->chip_index, cfg);
      data->f1 = Modulate_GetFhbfskFrequency(true, data->chip_index, cfg);
      GoertzelInfo_t goertzel_info;
      GoertzelInfoCopy(&goertzel_info, data);
      uint32_t f[2] = {data->f0, data->f1};
      float e_f[2];
      goertzel_info.f = f;
      goertzel_info.e_f = e_f;
      goertzel_info.energy_normalization = Demodulate_PowerNormalization();

      goertzel_2(&goertzel_info);

      data->analysis_done = true;
      data->decoded_bit = (goertzel_info.e_f[0] > goertzel_info.e_f[1]) ? false : true;
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
      uint16_t num_tones = (cfg->mod_demod_method == MOD_DEMOD_FSK) ? 1 : cfg->fhbfsk_num_tones;
      uint16_t frequency_index = data->bit_index % num_tones;

      uint16_t buffer_raw_length = data->bit_index / num_tones;
      uint16_t buffer_length = MIN(NUM_DEMODULATION_HISTORY, buffer_raw_length);
      uint16_t buffer_index = buffer_raw_length % NUM_DEMODULATION_HISTORY;

      // check conditions

      float delta_energy_f0, delta_energy_f1;
      float abs_normalized_delta_energy_f0, abs_normalized_delta_energy_f1;

      float energy_ratio = data->energy_f0 / data->energy_f1;

      if (buffer_length >= 1 && (energy_ratio < OVERWHELMING_ENERGY_THRESHOLD && energy_ratio > (1.0f / OVERWHELMING_ENERGY_THRESHOLD))) {
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
            abs_normalized_delta_energy_sum > significant_shift_threshold;

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

float Demodulate_PowerNormalization()
{
  switch (window_function) {
    case WINDOW_RECTANGULAR:
      return 1.0f;
    case WINDOW_HANN:
      return 1.63f;
    case WINDOW_HAMMING:
      return 1.59;
    default:
      return 1.0f;
  }
}

bool Demodulate_RegisterParams()
{
  uint32_t min_u32 = MIN_DEMOD_DECISION;
  uint32_t max_u32 = MAX_DEMOD_DECISION;
  if (Param_Register(PARAM_DEMODULATION_DECISION, "the demodulation method", PARAM_TYPE_UINT8,
                     &decision_method, sizeof(uint8_t), &min_u32, &max_u32, NULL) == false) {
    return false;
  }

  min_u32 = MIN_DEMOD_CAL_LOWER_F;
  max_u32 = MAX_DEMOD_CAL_LOWER_F;
  if (Param_Register(PARAM_DEMOD_CAL_LOWER_FREQ, "demod cal lower frequency", PARAM_TYPE_UINT32,
                     &lower_calibration_frequency, sizeof(uint32_t), &min_u32, &max_u32, NULL) == false) {
    return false;
  }

  min_u32 = MIN_DEMOD_CAL_LOWER_F;
  max_u32 = MAX_DEMOD_CAL_LOWER_F;
  if (Param_Register(PARAM_DEMOD_CAL_UPPER_FREQ, "demod cal upper frequency", PARAM_TYPE_UINT32,
                     &upper_calibration_frequency, sizeof(uint32_t), &min_u32, &max_u32, NULL) == false) {
    return false;
  }

  float min_f = MIN_HIST_CMP_THRESH;
  float max_f = MAX_HIST_CMP_THRESH;
  if (Param_Register(PARAM_HISTORICAL_COMPARISON_THRESHOLD, "significant shift threshold", PARAM_TYPE_FLOAT,
                     &significant_shift_threshold, sizeof(float), &min_f, &max_f, NULL) == false) {
    return false;
  }

  min_u32 = MIN_WINDOW_FUNCTION;
  max_u32 = MAX_WINDOW_FUNCTION;
  if (Param_Register(PARAM_WINDOW_FUNCTION, "windowing function", PARAM_TYPE_UINT8,
                     &window_function, sizeof(uint8_t), &min_u32, &max_u32, updateWindow) == false) {
    return false;
  }

  return true;
}

/* Private function definitions ----------------------------------------------*/

void GoertzelInfoCopy(GoertzelInfo_t* goertzel_info, DemodulationInfo_t* data)
{
  goertzel_info->buf_len = data->buf_len;
  goertzel_info->data_len = data->data_len;
  goertzel_info->start_pos = data->data_start_index;
  goertzel_info->window = window;
  goertzel_info->window_size = WINDOW_FUNCTION_SIZE;
}

void updateWindow()
{
  switch (window_function) {
    case WINDOW_RECTANGULAR:
      setWindowRectangular();
      break;
    case WINDOW_HANN:
      setWindowHann();
      break;
    case WINDOW_HAMMING:
      setWindowHamming();
      break;
    default:
      break;
  }
}

void setWindowRectangular()
{
  for (uint16_t i = 0; i < WINDOW_FUNCTION_SIZE; i++) {
    window[i] = 1.0f;
  }
}

void setWindowHann()
{
  // sin^2(pi*n/N)
  for (uint16_t i = 0; i < WINDOW_FUNCTION_SIZE; i++) {
    // pre-scaled by pi
    float angle = ((float) i) / ((float) (WINDOW_FUNCTION_SIZE - 1));
    float intermediate = uam_sinf(angle);
    window[i] = intermediate * intermediate;
  }
}

#define HAMMING_A0  (25.0f / 46.0f)
void setWindowHamming()
{
  // a0 - (1 - a0) * cos(2*pi*n/N)
  for (uint16_t i = 0; i < WINDOW_FUNCTION_SIZE; i++) {
    float angle = ((float) 2 * i) / ((float) (WINDOW_FUNCTION_SIZE - 1));
    float intermediate = uam_cosf(angle);
    window[i] = HAMMING_A0 - (1 - HAMMING_A0) * intermediate;
  }
}
