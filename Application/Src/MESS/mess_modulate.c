/*
 * mess_modulate.c
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "dac_waveform.h"
#include "mess_adc.h"
#include "cmsis_os.h"
#include "mess_packet.h"
#include "mess_feedback.h"
#include "mess_dsp_config.h"
#include "mess_dac_resources.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static float output_amplitude = DEFAULT_OUTPUT_AMPLITUDE;

static WaveformStep_t test_sequence[2];

static uint32_t test_freq = 30000;

static OutputStrengthMethod_t output_strength_method = DEFAULT_MOD_OUTPUT_METHOD;

static float target_power_w = DEFAULT_MOD_TARGET_POWER;

static float motional_head_r_ohm = DEFAULT_R;
static float motional_head_c0_nf = DEFAULT_C0;
static float motional_head_l0_mh = DEFAULT_L0;
static float parallel_c1_nf = DEFAULT_C1;

static float max_transducer_voltage = DEFAULT_MAX_TRANSDUCER_V;

static const uint8_t galois_matrix[3][12] = {
  {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
  {2,  4,  8,  3,  6, 12, 11,  9,  5, 10,  7,  1},
  {4,  3, 12,  9, 10,  1,  4,  3, 12,  9, 10,  1}
};

extern const uint16_t primes[50];
static const uint16_t num_primes = sizeof(primes) / sizeof(primes[0]);

/* Private function prototypes -----------------------------------------------*/

uint32_t getFhbfskSeqeunceNumber(uint32_t normalized_bit_index, const DspConfig_t* cfg);
uint32_t incrementSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences);
uint32_t galoisSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences);
uint32_t primeSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences);
bool isPrime(uint16_t num);

/* Exported function definitions ---------------------------------------------*/

float Modulate_GetAmplitude(uint32_t freq_hz)
{
  (void)(freq_hz);
  return output_amplitude;
}

bool Modulate_StartTransducerOutput(uint16_t num_steps, 
                                    const DspConfig_t* new_cfg, 
                                    BitMessage_t* new_bit_msg)
{
  HAL_TIM_Base_Stop(&htim6);
  ADC_StopAll();
  Waveform_StopWaveformOutput();
  osDelay(1);
  MessDacResource_RegisterMessageConfiguration(new_cfg, new_bit_msg);
  Waveform_SetWaveformSequence(num_steps);
  if (ADC_StartFeedback() == false) {
    return false;
  }
  if (Waveform_StartWaveformOutput(DAC_CHANNEL_TRANSDUCER) == false) {
    return false;
  }
  osDelay(150);
  return HAL_TIM_Base_Start(&htim6) == HAL_OK;
}

bool Modulate_StartFeedbackOutput(uint16_t num_steps, 
                                  const DspConfig_t* new_cfg, 
                                  BitMessage_t* new_bit_msg)
{
  HAL_TIM_Base_Stop(&htim6);
  Waveform_StopWaveformOutput();
  osDelay(1);
  MessDacResource_RegisterMessageConfiguration(new_cfg, new_bit_msg);
  Waveform_SetWaveformSequence(num_steps);
  if (Waveform_StartWaveformOutput(DAC_CHANNEL_FEEDBACK) == false) {
    return false;
  }
  HAL_StatusTypeDef ret = HAL_TIM_Base_Start(&htim6);
  return ret == HAL_OK;
}

// TODO: properly deprecate
void Modulate_TestOutput()
{
  test_sequence[0].duration_us = 1000;
  test_sequence[0].freq_hz = 30000;
  test_sequence[0].relative_amplitude = output_amplitude;
  test_sequence[1].duration_us = 1000;
  test_sequence[1].freq_hz = 33000;
  test_sequence[1].relative_amplitude = output_amplitude;

//  Waveform_SetWaveformSequence(test_sequence, 2);
}

// TODO: properly deprecate
void Modulate_TestFrequencyResponse()
{
  test_sequence[0].duration_us = FEEDBACK_TEST_DURATION_MS * 1000;
  test_sequence[0].freq_hz = test_freq;
  test_sequence[0].relative_amplitude = output_amplitude;

//  Waveform_SetWaveformSequence(test_sequence, 1);
}

uint32_t Modulate_GetFhbfskFrequency(bool bit, 
                                     uint16_t bit_index, 
                                     const DspConfig_t* cfg)
{
  uint32_t frequency_separation = cfg->fhbfsk_freq_spacing * cfg->baud_rate;

  uint32_t start_freq = cfg->fc - 
      frequency_separation * (2 * cfg->fhbfsk_num_tones - 1) / 2;
  start_freq = (start_freq / frequency_separation) * frequency_separation;

  uint32_t sequence_number = getFhbfskSeqeunceNumber(bit_index / cfg->fhbfsk_dwell_time, cfg);
  uint32_t frequency_index = 2 * sequence_number + bit;
  return start_freq + frequency_separation * frequency_index;
}

uint32_t Modulate_GetFskFrequency(bool bit, const DspConfig_t* cfg)
{
  return (bit) ? cfg->fsk_f1 : cfg->fsk_f0;
}

bool Modulate_RegisterParams()
{
  float min_f = MIN_OUTPUT_AMPLITUDE;
  float max_f = MAX_OUTPUT_AMPLITUDE;
  if (Param_Register(PARAM_OUTPUT_AMPLITUDE, "output amplitude", PARAM_TYPE_FLOAT,
                     &output_amplitude, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  uint32_t min_u32 = MIN_MOD_OUTPUT_METHOD;
  uint32_t max_u32 = MAX_MOD_OUTPUT_METHOD;
  if (Param_Register(PARAM_MODULATION_OUTPUT_METHOD, "output strength method", PARAM_TYPE_UINT8,
                     &output_strength_method, sizeof(uint8_t), &min_u32, &max_u32) == false) {
    return false;
  }

  min_f = MIN_MOD_TARGET_POWER;
  max_f = MAX_MOD_TARGET_POWER;
  if (Param_Register(PARAM_MODULATION_TARGET_POWER, "target output power", PARAM_TYPE_FLOAT,
                     &target_power_w, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  min_f = MIN_R;
  max_f = MAX_R;
  if (Param_Register(PARAM_R, "motional head R [ohm]", PARAM_TYPE_FLOAT,
                     &motional_head_r_ohm, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  min_f = MIN_C0;
  max_f = MAX_C0;
  if (Param_Register(PARAM_C0, "motional head C0 [nF]", PARAM_TYPE_FLOAT,
                     &motional_head_c0_nf, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  min_f = MIN_L0;
  max_f = MAX_L0;
  if (Param_Register(PARAM_L0, "motional head L0 [mH]", PARAM_TYPE_FLOAT,
                     &motional_head_l0_mh, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  min_f = MIN_C1;
  max_f = MAX_C1;
  if (Param_Register(PARAM_C1, "parallel cap c1 [nF]", PARAM_TYPE_FLOAT,
                     &parallel_c1_nf, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  min_f = MIN_MAX_TRANSDUCER_V;
  max_f = MAX_MAX_TRANSDUCER_V;
  if (Param_Register(PARAM_MAX_TRANSDUCER_VOLTAGE, "Maximum transducer voltage", PARAM_TYPE_FLOAT,
                     &max_transducer_voltage, sizeof(float), &min_f, &max_f) == false) {
    return false;
  }

  return true;
}


/* Private function definitions ----------------------------------------------*/

uint32_t getFhbfskSeqeunceNumber(uint32_t normalized_bit_index, const DspConfig_t* cfg)
{
  switch (cfg->fhbfsk_hopper) {
    case HOPPER_INCREMENT:
      return incrementSequenceNumber(normalized_bit_index, cfg->fhbfsk_num_tones);
    case HOPPER_GALOIS:
      return galoisSequenceNumber(normalized_bit_index, cfg->fhbfsk_num_tones);
    case HOPPER_PRIME:
      return primeSequenceNumber(normalized_bit_index, cfg->fhbfsk_num_tones);
    default:
      return incrementSequenceNumber(normalized_bit_index, cfg->fhbfsk_num_tones);
  }
}

uint32_t incrementSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences)
{
  return normalized_bit_index % num_sequences;
}

// TODO: generalize to any prime number of sequences
uint32_t galoisSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences)
{
  if (num_sequences != 13) {
    return incrementSequenceNumber(normalized_bit_index, num_sequences);
  }

  uint16_t i = (normalized_bit_index / 12) % (13 * 12);
  uint16_t j = normalized_bit_index % 12;
  uint16_t Pi[3] = {0, i / 13 + 1, i % 13};

  uint32_t sequence_number = 0;

  for (uint8_t k = 0; k < 3; k++) {
    sequence_number += Pi[k] * galois_matrix[k][j];
  }

  return sequence_number % 13;
}

uint32_t primeSequenceNumber(uint32_t normalized_bit_index, uint16_t num_sequences)
{
  // Cache variables initialized to "unset"
  static uint32_t last_num_sequences = 0;
  static uint16_t last_hop_amount = 0;

  // Check if either unset cache or if there is a cache hit
  if (last_num_sequences != num_sequences || last_num_sequences == 0) {
    // Compute the hop amount for a given sequence number
    last_num_sequences = num_sequences;
    if (isPrime(num_sequences) == true) {
      // Already one is prime so maximize orthogonality (power of 2 hopping)
      for (uint16_t i = 0; i < 8; i++) {
        uint16_t candidate = 1 << i;
        if (candidate > num_sequences) {
          last_hop_amount = candidate;
          break;
        }
        if (candidate * candidate >= num_sequences - 1) {
          last_hop_amount = 1;
          break;
        }
        last_hop_amount = 1;
      }
    } else {
      // Need to find a suitable prime number hopping depth
      for (uint16_t i = 0; i < num_primes; i++) {
        if (primes[i] >= num_sequences) {
          last_hop_amount = 1;
        }
        if ((primes[i] * primes[i] > num_sequences) && (num_sequences % primes[i] != 0)) {
          last_hop_amount = primes[i];
        }
      }
    }
  }

  return (last_hop_amount * normalized_bit_index) % num_sequences;
}

bool isPrime(uint16_t num)
{
  for (uint16_t i = 0; i < num_primes; i++) {
    if (primes[i] == num) {
      return true;
    }
    if (primes[i] > num) {
      break;
    }
  }
  return false;
}
