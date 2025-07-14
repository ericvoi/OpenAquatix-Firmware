/*
 * mess_adc.c
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_adc.h"
#include "mess_input.h"
#include "mess_feedback.h"
#include "sys_temperature.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include "FreeRTOS.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define INPUT_ADC         hadc2
#define FEEDBACK_ADC      hadc1

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t adc_buffer[ADC_BUFFER_SIZE] __attribute__((section(".dma_buf"))); // shared ADC buffer for both feedback and input ADCs

volatile uint16_t input_head_pos = 0;
volatile uint16_t input_tail_pos = 0;

volatile uint16_t feedback_head_pos = 0;
volatile uint16_t feedback_tail_pos = 0;

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

static bool input_sample_lost = false;
static bool feedback_sample_lost = false;

 // Number of times the head of either buffer has wrapped around to 0
static uint16_t buffer_rollover_count = 0;
static uint32_t rollover_associated_time;

/* Private function prototypes -----------------------------------------------*/

void addToInputBuffer(bool firstHalf);
void addToFeedbackBuffer(bool firstHalf);
void incrementRollover();

/* Exported function definitions ---------------------------------------------*/

bool ADC_Init()
{
  HAL_StatusTypeDef ret1 = HAL_TIM_Base_Start(&htim8);

  input_head_pos = 0;
  input_tail_pos = 0;
  feedback_head_pos = 0;
  feedback_tail_pos = 0;

  memset(adc_buffer, 0, ADC_BUFFER_SIZE * sizeof(uint16_t));

  return ret1 == HAL_OK;
}

bool ADC_StartInput()
{
  input_head_pos = 0;
  input_tail_pos = 0;
  HAL_TIM_Base_Start(&htim8);
  HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&INPUT_ADC, (uint32_t*) adc_buffer, ADC_BUFFER_SIZE);
  return ret == HAL_OK;
}

bool ADC_StartFeedback()
{
  feedback_head_pos = 0;
  feedback_tail_pos = 0;
  HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&FEEDBACK_ADC, (uint32_t*) adc_buffer, ADC_BUFFER_SIZE);
  return ret == HAL_OK;
}

bool ADC_StopFeedback()
{
  return HAL_ADC_Stop_DMA(&FEEDBACK_ADC) == HAL_OK;
}

bool ADC_StopInput()
{
  return HAL_ADC_Stop_DMA(&INPUT_ADC) == HAL_OK;
}

bool ADC_StopAll()
{
  if (ADC_StopFeedback() == false) {
    return false;
  }
  if (ADC_StopInput() == false) {
    return false;
  }
  return true;
}

void ADC_InputClear()
{
  input_head_pos = 0;
  input_tail_pos = 0;
  buffer_rollover_count = 0;
  memset(input_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(float));
}

void ADC_FeedbackClear()
{
  feedback_head_pos = 0;
  feedback_tail_pos = 0;
  buffer_rollover_count = 0;
  memset(feedback_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(uint16_t));
}

uint16_t ADC_HeadRolloverCount()
{
  return buffer_rollover_count;
}

uint16_t ADC_TailRolloverCount(bool feedback)
{
  uint16_t head;
  uint16_t tail;
  if (feedback == true) {
    head = feedback_head_pos;
    tail = feedback_tail_pos;
  }
  else {
    head = input_head_pos;
    tail = input_tail_pos;
  }
  if (head >= tail) {
    return buffer_rollover_count;
  }
  else { // head has rolled over while the tail has not
    return buffer_rollover_count - 1;
  }
}

/* Private function definitions ----------------------------------------------*/

void addToInputBuffer(bool firstHalf)
{
  uint16_t dma_buf_start_index = (firstHalf == true) ? (0) : (ADC_BUFFER_SIZE / 2);

  // Check for overflowing buffer
  uint16_t unprocessed_samples = (input_head_pos - input_tail_pos) & PROCESSING_BUFFER_MASK;
  if ((unprocessed_samples + ADC_BUFFER_SIZE / 2) > PROCESSING_BUFFER_SIZE) {
    input_sample_lost = true;
  }
  uint16_t original_head = input_head_pos;

  for (uint16_t i = 0; i < ADC_BUFFER_SIZE / 2; i++) {
    uint16_t dma_index = dma_buf_start_index + i;
    input_buffer[input_head_pos] = (float) adc_buffer[dma_index];
    input_head_pos = (input_head_pos + 1) & PROCESSING_BUFFER_MASK;
  }
  if (original_head > input_head_pos) {
    incrementRollover();
  }
}

void addToFeedbackBuffer(bool firstHalf)
{
  uint16_t dma_buf_start_index = (firstHalf == true) ? (0) : (ADC_BUFFER_SIZE / 2);

  // Check for overflowing buffer
  uint16_t unprocessed_samples = (feedback_head_pos - input_tail_pos) & PROCESSING_BUFFER_MASK;
  if ((unprocessed_samples + ADC_BUFFER_SIZE / 2) > PROCESSING_BUFFER_SIZE) {
    feedback_sample_lost = true;
  }
  uint16_t original_head = feedback_head_pos;

  if (feedback_head_pos + ADC_BUFFER_SIZE / 2 > PROCESSING_BUFFER_SIZE) {
    uint16_t first_block_size = PROCESSING_BUFFER_SIZE - feedback_head_pos;
    memcpy(&feedback_buffer[feedback_head_pos], &adc_buffer[dma_buf_start_index], first_block_size * sizeof(uint16_t));
    uint16_t second_block_size = ADC_BUFFER_SIZE / 2 - first_block_size;
    memcpy(&feedback_buffer[0], &adc_buffer[first_block_size + dma_buf_start_index], second_block_size * sizeof(uint16_t));
  }
  else {
    memcpy(&feedback_buffer[feedback_head_pos], &adc_buffer[dma_buf_start_index], (ADC_BUFFER_SIZE / 2) * sizeof(uint16_t));
  }

  feedback_head_pos = (feedback_head_pos + ADC_BUFFER_SIZE / 2) & PROCESSING_BUFFER_MASK;
  if (original_head > feedback_head_pos) {
    incrementRollover();
  }
}

void incrementRollover()
{
  buffer_rollover_count++;
  rollover_associated_time = 0; // TODO: change to a timer
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc == &INPUT_ADC) {
    addToInputBuffer(true);
  }
  else if (hadc == &FEEDBACK_ADC) {
    addToFeedbackBuffer(true);
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
  if (hadc == &INPUT_ADC) {
    addToInputBuffer(false);
  }
  else if (hadc == &FEEDBACK_ADC) {
    addToFeedbackBuffer(false);
  }
  else if (hadc == &TEMPERATURE_ADC) {
    Temperature_AddValue();
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
  if (hadc == &INPUT_ADC) {
    return;
  }
}
