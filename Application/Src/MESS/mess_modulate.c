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

/* Private function prototypes -----------------------------------------------*/

bool convertToFrequencyFsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence);
bool convertToFrequencyFhbfsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence);
uint32_t getFskFrequency(bool bit);

/* Exported function definitions ---------------------------------------------*/

bool Modulate_ConvertToFrequency(BitMessage_t* bit_msg, WaveformStep_t* message_sequence)
{
  switch (mod_demod_method) {
    case MOD_DEMOD_FSK:
      return convertToFrequencyFsk(bit_msg, message_sequence);
      break;
    case MOD_DEMOD_FHBFSK:
      return convertToFrequencyFhbfsk(bit_msg, message_sequence);
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

bool Modulate_ApplyDuration(WaveformStep_t* message_sequence, uint16_t len)
{
  for (uint16_t i = 0; i < len; i++) {
    message_sequence[i].duration_us = (uint32_t) roundf(1000000.0f / baud_rate);
  }
  return true;
}

float Modulate_GetTransducerAmplitude(void)
{
  return output_amplitude;
}

void Modulate_ChangeTransducerAmplitude(float new_amplitude)
{
  output_amplitude = new_amplitude;
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

void Modulate_SetTestFrequency(uint32_t freq_hz)
{
  test_freq = freq_hz;
}

void Modulate_TestFrequencyResponse()
{
  test_sequence[0].duration_us = FEEDBACK_TEST_DURATION_MS * 1000;
  test_sequence[0].freq_hz = test_freq;
  test_sequence[0].relative_amplitude = output_amplitude;

  DAC_SetWaveformSequence(test_sequence, 1);
}

uint32_t Modulate_GetFhbfskFrequency(bool bit, uint16_t bit_index)
{
  uint32_t frequency_separation = fhbfsk_freq_spacing * baud_rate;

  uint32_t start_freq = fc - frequency_separation * (2 * fhbfsk_num_tones - 1) / 2;
  start_freq = (start_freq / frequency_separation) * frequency_separation;

  uint32_t frequency_index = 2 * ((bit_index / fhbfsk_dwell_time) % fhbfsk_num_tones);
  frequency_index += bit;
  return start_freq + frequency_separation * frequency_index;
}

bool Modulate_RegisterParams()
{
  float min = MIN_OUTPUT_AMPLITUDE;
  float max = MAX_OUTPUT_AMPLITUDE;
  if (Param_Register(PARAM_OUTPUT_AMPLITUDE, "output amplitude", PARAM_TYPE_FLOAT,
                     &output_amplitude, sizeof(float), &min, &max) == false) {
    return false;
  }

  return true;
}


/* Private function definitions ----------------------------------------------*/

bool convertToFrequencyFsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence)
{
  for (uint16_t i = 0; i < bit_msg->bit_count; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, i, &bit) == false) {
      return false;
    }
    message_sequence[i].freq_hz = getFskFrequency(bit);
  }
  return true;
}

bool convertToFrequencyFhbfsk(BitMessage_t* bit_msg, WaveformStep_t* message_sequence)
{
  for (uint16_t i = 0; i < bit_msg->bit_count; i++) {
    bool bit;
    if (Packet_GetBit(bit_msg, i, &bit) == false) {
      return false;
    }
    message_sequence[i].freq_hz = Modulate_GetFhbfskFrequency(bit, i);
  }
  return true;
}

uint32_t getFskFrequency(bool bit)
{
  return (bit) ? fsk_f1 : fsk_f0;
}
