/*
 * mess_background_noise.c
 *
 *  Created on: Jul 14, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_background_noise.h"
#include "mess_adc.h"
#include "mess_demodulate.h"
#include "arm_math.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  float accumulated_energy;
  uint16_t counts;
} EnergyHistory_t;

/* Private define ------------------------------------------------------------*/

#define NOISE_BUFFER_SIZE   128
#define MS_PER_ENTRY        100
#define COUNTS_PER_ENTRY    (ADC_SAMPLING_RATE * MS_PER_ENTRY / 1000 / NOISE_BUFFER_SIZE)
#define NOISE_HISTORY_SIZE  10

#define NUM_NOISE_IN_AVERAGE  30

#define WINDOW_INCREMENT      4

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

extern arm_rfft_fast_instance_f32 fft_handle128;

static bool energy_ready = false;

static EnergyHistory_t energy_history[NOISE_HISTORY_SIZE];

static volatile float in_band_noise = 0.0f;
static uint16_t noise_buffer_tail = 0;
static uint16_t noise_history_index = 0;
static uint16_t accumulated_noise_entries = 0;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void BackgroundNoise_Reset()
{
  noise_buffer_tail = 0;
  accumulated_noise_entries = 0;
  noise_history_index = 0;
  in_band_noise = 0.0f;
  energy_ready = false;
}

void BackgroundNoise_Calculate()
{
  uint16_t head = ADC_InputGetHead();
  while (((head - noise_buffer_tail) & PROCESSING_BUFFER_MASK) > NOISE_BUFFER_SIZE) {
    float fft_in_buf[NOISE_BUFFER_SIZE];
    float fft_out_buf[NOISE_BUFFER_SIZE];

    for (uint16_t i = 0; i < NOISE_BUFFER_SIZE; i++) {
      uint16_t index = (noise_buffer_tail + i) & PROCESSING_BUFFER_MASK;
      fft_in_buf[i] = ADC_InputGetDataAbsolute(index);
    }
    arm_rfft_fast_f32(&fft_handle128, fft_in_buf, fft_out_buf, 0);

    for (uint16_t j = 28; j < 39; j++) {
      float real = fft_out_buf[2 * j];
      float imag = fft_out_buf[2 * j + 1];

      // Normalize to PSD (P/Hz)
      float mag = (real * real + imag * imag) / NOISE_BUFFER_SIZE;
      
      energy_history[noise_history_index].accumulated_energy += mag / 12;
    }
    energy_history[noise_history_index].counts++;
    if (energy_history[noise_history_index].counts >= COUNTS_PER_ENTRY) {
      if (noise_history_index == 9 || accumulated_noise_entries != 0) {
        float energy = energy_history[noise_history_index].accumulated_energy / energy_history[noise_history_index].counts;
        in_band_noise = (in_band_noise * accumulated_noise_entries + energy) / (accumulated_noise_entries + 1);
        accumulated_noise_entries = MIN(accumulated_noise_entries + 1, NUM_NOISE_IN_AVERAGE);
        energy_ready = accumulated_noise_entries == NUM_NOISE_IN_AVERAGE;
      }
      noise_history_index = (noise_history_index + 1) % NOISE_HISTORY_SIZE;
      energy_history[noise_history_index].counts = 0;
      energy_history[noise_history_index].accumulated_energy = 0.0f;
    }
    noise_buffer_tail = (noise_buffer_tail + NOISE_BUFFER_SIZE) & PROCESSING_BUFFER_MASK;
  }
}

float BackgroundNoise_Get()
{
  return in_band_noise;
}

/* Private function definitions ----------------------------------------------*/
