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
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "stm32h7xx_hal.h"

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

/* Private function prototypes -----------------------------------------------*/

bool convertToFrequencyFsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence, const DspConfig_t* cfg);
bool convertToFrequencyFhbfsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence, const DspConfig_t* cfg);
uint32_t getFskFrequency(bool bit, const DspConfig_t* cfg);

/* Exported function definitions ---------------------------------------------*/

bool Modulate_ConvertToFrequency(BitMessage_t* bit_msg, WaveformStep_t* message_sequence, const DspConfig_t* cfg)
{
  switch (cfg->mod_demod_method) {
    case MOD_DEMOD_FSK:
      return convertToFrequencyFsk(bit_msg, message_sequence, cfg);
      break;
    case MOD_DEMOD_FHBFSK:
      return convertToFrequencyFhbfsk(bit_msg, message_sequence, cfg);
      break;
    default:
      break;
  }
  return true;
}

bool Modulate_ApplyAmplitude(WaveformStep_t* message_sequence, uint16_t len)
{
  float amplitude = output_amplitude;
  for (uint16_t i = 0; i < len; i++) {
    message_sequence[i].relative_amplitude = amplitude;
  }
  return true;
}

bool Modulate_ApplyDuration(WaveformStep_t* message_sequence, uint16_t len, const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < len; i++) {
    message_sequence[i].duration_us = (uint32_t) roundf(1000000.0f / cfg->baud_rate);
  }
  return true;
}

bool Modulate_StartTransducerOutput()
{
  HAL_TIM_Base_Stop(&htim6);
  ADC_StopAll();
  DAC_StopWaveformOutput();
  osDelay(1);
  if (ADC_StartFeedback() == false) {
    return false;
  }
  if (DAC_StartWaveformOutput(DAC_CHANNEL_1) == false) {
    return false;
  }
  osDelay(150);
  HAL_StatusTypeDef ret = HAL_TIM_Base_Start(&htim6);
  return ret == HAL_OK;
}

void Modulate_TestOutput()
{
  test_sequence[0].duration_us = 1000;
  test_sequence[0].freq_hz = 30000;
  test_sequence[0].relative_amplitude = output_amplitude;
  test_sequence[1].duration_us = 1000;
  test_sequence[1].freq_hz = 33000;
  test_sequence[1].relative_amplitude = output_amplitude;

  DAC_SetWaveformSequence(test_sequence, 2);
}

void Modulate_TestFrequencyResponse()
{
  test_sequence[0].duration_us = FEEDBACK_TEST_DURATION_MS * 1000;
  test_sequence[0].freq_hz = test_freq;
  test_sequence[0].relative_amplitude = output_amplitude;

  DAC_SetWaveformSequence(test_sequence, 1);
}

uint32_t Modulate_GetFhbfskFrequency(bool bit, uint16_t bit_index, const DspConfig_t* cfg)
{
  uint32_t frequency_separation = cfg->fhbfsk_freq_spacing * cfg->baud_rate;

  uint32_t start_freq = cfg->fc - frequency_separation * (2 * cfg->fhbfsk_num_tones - 1) / 2;
  start_freq = (start_freq / frequency_separation) * frequency_separation;

  uint32_t frequency_index = 2 * ((bit_index / cfg->fhbfsk_dwell_time) % cfg->fhbfsk_num_tones);
  frequency_index += bit;
  return start_freq + frequency_separation * frequency_index;
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

bool convertToFrequencyFsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence, const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < bit_msg->bit_count; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, i, &bit) == false) {
      return false;
    }
    message_sequence[i].freq_hz = getFskFrequency(bit, cfg);
  }
  return true;
}

bool convertToFrequencyFhbfsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence, const DspConfig_t* cfg)
{
  for (uint16_t i = 0; i < bit_msg->bit_count; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, i, &bit) == false) {
      return false;
    }
    message_sequence[i].freq_hz = Modulate_GetFhbfskFrequency(bit, i, cfg);
  }
  return true;
}

uint32_t getFskFrequency(bool bit, const DspConfig_t* cfg)
{
  return (bit) ? cfg->fsk_f1 : cfg->fsk_f0;
}
