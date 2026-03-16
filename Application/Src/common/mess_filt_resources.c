/*
 * mess_filt_resources.c
 *
 *  Created on: Feb 24, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_filt_resources.h"
#include "main.h"
#include "filt_main.h"
#include "error_manager.h"
#include "stm32h7xx_hal.h"
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint32_t cyccnt;
  uint32_t associated_rollover_count;
  uint16_t associated_sample;
} CyccntTracker_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/

#define MIN(a, b) ((a < b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

/*
 * Only one ADC runs at a time and both have significant DSP operations done on
 * them so they benefit greatly from DTCM buffers. DTCM memory is limited, so
 * they share a buffer
 */
typedef union {
  float in_buf[PROCESSING_BUFFER_SIZE];
  uint16_t fb_buf[PROCESSING_BUFFER_SIZE];
} AdcBuffers_t;

static AdcBuffers_t adc_buffers __attribute__((section(".dtcm")));

float* input_buffer = adc_buffers.in_buf;
uint16_t* feedback_buffer = adc_buffers.fb_buf;

volatile uint16_t adc_buffer[ADC_BUFFER_SIZE] __attribute__((section(".dma_buf"))); // shared ADC buffer for both feedback and input ADCs

volatile uint16_t input_head_pos = 0;
volatile uint16_t input_processing_tail_pos = 0;
volatile uint16_t input_noise_tail_pos = 0;

volatile uint16_t feedback_head_pos = 0;
volatile uint16_t feedback_tail_pos = 0;

// Number of times the head of either buffer has wrapped around to 0
static uint16_t buffer_rollover_count = 0;
static uint64_t rollover_associated_time;

// TODO: fix issue with multiple threads accessing this
static CyccntTracker_t cyccnt_tracker;

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern TIM_HandleTypeDef htim8;

/* Private function prototypes -----------------------------------------------*/

static void incrementRollover(void);

/* Exported function definitions ---------------------------------------------*/

void MessFiltResources_Init()
{
  RETURN_IF_ERROR_PRESENT();
  HAL_TIM_Base_Stop(&htim8);
  HAL_TIM_Base_Start(&htim8);

  input_head_pos = 0;
  input_processing_tail_pos = 0;
  input_noise_tail_pos = 0;
  feedback_head_pos = 0;
  feedback_tail_pos = 0;

  memset((void*) adc_buffer, 0, ADC_BUFFER_SIZE * sizeof(uint16_t));
}

void MessFiltResources_StartInputAdc()
{
  RETURN_IF_ERROR_PRESENT();
  input_head_pos = 0;
  input_processing_tail_pos = 0;
  input_noise_tail_pos = 0;
  HAL_TIM_Base_Stop(&htim8);
  HAL_ADC_Stop_DMA(&INPUT_ADC);
  osDelay(1);
  HAL_TIM_Base_Start(&htim8);
  HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&INPUT_ADC, (uint32_t*) adc_buffer, ADC_BUFFER_SIZE);
  if (ret != HAL_OK) 
    REGISTER_ERROR(ERROR_INPUT_ADC_INITIALIZATION);
}

void MessFiltResources_StartFeedbackAdc()
{
  feedback_head_pos = 0;
  feedback_tail_pos = 0;
  HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&FEEDBACK_ADC, (uint32_t*) adc_buffer, ADC_BUFFER_SIZE);
  if (ret != HAL_OK) 
    REGISTER_ERROR(ERROR_FEEDBACK_ADC_INITIALIZATION);
}

bool MessFiltResources_StopFeedbackAdc()
{
  return HAL_ADC_Stop_DMA(&FEEDBACK_ADC) == HAL_OK;
}

bool MessFiltResources_StopInputAdc()
{
  return HAL_ADC_Stop_DMA(&INPUT_ADC) == HAL_OK;
}

bool MessFiltResources_StopAllAdcs()
{
  if (MessFiltResources_StopFeedbackAdc() == false) {
    return false;
  }
  if (MessFiltResources_StopInputAdc() == false) {
    return false;
  }
  return true;
}

void MessFiltResources_InputAdcClear()
{
  input_head_pos = 0;
  input_processing_tail_pos = 0;
  input_noise_tail_pos = 0;
  buffer_rollover_count = 0;
  memset(input_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(float));
}

void MessFiltResources_FeedbackAdcClear()
{
  feedback_head_pos = 0;
  feedback_tail_pos = 0;
  buffer_rollover_count = 0;
  memset(feedback_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(uint16_t));
}

void MessFiltResources_AddFilteredSamples(float* buf, uint16_t num_samples, uint32_t cyccnt)
{
  uint16_t processing_len = (input_head_pos - input_processing_tail_pos) & PROCESSING_BUFFER_MASK;
  // uint16_t noise_len      = (input_head_pos - input_noise_tail_pos)      & PROCESSING_BUFFER_MASK;

  uint16_t processing_len_left = PROCESSING_BUFFER_SIZE - processing_len;
  // uint16_t noise_len_left      = PROCESSING_BUFFER_SIZE - noise_len;

  if (processing_len_left <= num_samples /*|| noise_len_left <= num_samples*/) {
    // TODO: handle error
    return;
  }

  uint16_t chunk1_size = MIN(num_samples, PROCESSING_BUFFER_SIZE - input_head_pos);
  uint16_t chunk2_size = num_samples - chunk1_size;

  memcpy(&input_buffer[input_head_pos], buf, chunk1_size * sizeof(float));
  memcpy(&input_buffer[0], &buf[chunk1_size], chunk2_size * sizeof(float));

  input_head_pos = (input_head_pos + num_samples) & PROCESSING_BUFFER_MASK;

  if (chunk2_size != 0 || input_head_pos == 0) incrementRollover();

  cyccnt_tracker.associated_sample = input_head_pos;
  cyccnt_tracker.associated_rollover_count = buffer_rollover_count;
  cyccnt_tracker.cyccnt = cyccnt;
}

uint32_t MessFiltResources_AssociatedCyccnt(uint16_t position, uint32_t rollovers)
{
  uint64_t absolute_reference_index = 
      (cyccnt_tracker.associated_rollover_count << PROCESSING_BUFFER_POWER) | 
      (cyccnt_tracker.associated_sample);
  uint64_t absolute_input_index = (rollovers << PROCESSING_BUFFER_POWER) | 
                                   position;

  if (absolute_reference_index < absolute_input_index) return 0;

  uint32_t index_difference = absolute_reference_index - absolute_input_index;

  uint32_t cyccnt_difference = 
      ((uint64_t) index_difference * (uint64_t) SystemCoreClock) / 
      (FILT_GetSamplingRate());

  return cyccnt_tracker.cyccnt - cyccnt_difference;
}

uint32_t MessFiltResources_HeadRolloverCount()
{
  return buffer_rollover_count;
}

uint32_t MessFiltResources_TailRolloverCount(bool feedback)
{
  uint16_t head;
  uint16_t tail;
  if (feedback == true) {
    head = feedback_head_pos;
    tail = feedback_tail_pos;
  }
  else {
    head = input_head_pos;
    tail = input_processing_tail_pos; // Rollover only matters for processing tail
  }
  if (head >= tail) {
    return buffer_rollover_count;
  }
  else { // head has rolled over while the tail has not
    return buffer_rollover_count - 1;
  }
}

/* Private function definitions ----------------------------------------------*/

void incrementRollover()
{
  buffer_rollover_count++;
  rollover_associated_time = HAL_AbsoluteTimestamp();
}
