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

static uint16_t adc_buffer[ADC_BUFFER_SIZE]; // shared ADC buffer for both feedback and input ADCs
static uint16_t* input_buffer = NULL;
static uint16_t* feedback_buffer = NULL;

static uint16_t input_buffer_index = 0;
static uint16_t feedback_buffer_index = 0;

/* Private function prototypes -----------------------------------------------*/

void addToInputBuffer(bool firstHalf);
void addToFeedbackBuffer(bool firstHalf);

/* Exported function definitions ---------------------------------------------*/

bool ADC_Init()
{
  HAL_StatusTypeDef ret1 = HAL_TIM_Base_Start(&htim8);

  input_buffer_index = 0;
  feedback_buffer_index = 0;

  memset(adc_buffer, 0, ADC_BUFFER_SIZE * sizeof(uint16_t));

  return ret1 == HAL_OK;
}

bool ADC_RegisterInputBuffer(uint16_t* in_buffer)
{
  if (input_buffer != NULL) return false;

  input_buffer = in_buffer;
  return true;
}

bool ADC_RegisterFeedbackBuffer(uint16_t* fb_buffer)
{
  if (feedback_buffer != NULL) return false;

  feedback_buffer = fb_buffer;
  return true;
}

bool ADC_StartInput()
{
  input_buffer_index = 0;
  HAL_TIM_Base_Start(&htim8);
  HAL_StatusTypeDef ret = HAL_ADC_Start_DMA(&INPUT_ADC, (uint32_t*) adc_buffer, ADC_BUFFER_SIZE);
  return ret == HAL_OK;
}

bool ADC_StartFeedback()
{
  feedback_buffer_index = 0;
  if (feedback_buffer == NULL) return false;
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
  feedback_buffer_index = 0;
  input_buffer_index = 0;
  return true;
}

/* Private function definitions ----------------------------------------------*/

void addToInputBuffer(bool firstHalf)
{
  if (input_buffer == NULL) return;

  uint16_t dma_buf_start_index = (firstHalf == true) ? (0) : (ADC_BUFFER_SIZE / 2);

  if (input_buffer_index >= PROCESSING_BUFFER_SIZE) {
    input_buffer_index = input_buffer_index % PROCESSING_BUFFER_SIZE;
  }

  if (input_buffer_index + ADC_BUFFER_SIZE / 2 > PROCESSING_BUFFER_SIZE) {
    uint16_t first_block_size = PROCESSING_BUFFER_SIZE - input_buffer_index;
    memcpy(&input_buffer[input_buffer_index], &adc_buffer[dma_buf_start_index], first_block_size * sizeof(uint16_t));
    uint16_t second_block_size = ADC_BUFFER_SIZE / 2 - first_block_size;
    memcpy(&input_buffer[0], &adc_buffer[first_block_size + dma_buf_start_index], second_block_size * sizeof(uint16_t));
  }
  else {
    memcpy(&input_buffer[input_buffer_index], &adc_buffer[dma_buf_start_index], (ADC_BUFFER_SIZE / 2) * sizeof(uint16_t));
  }

  input_buffer_index = (input_buffer_index + ADC_BUFFER_SIZE / 2) % PROCESSING_BUFFER_SIZE;

  Input_IncrementEndIndex();
}

void addToFeedbackBuffer(bool firstHalf)
{
  if (feedback_buffer == NULL) return;

  uint16_t dma_buf_start_index = (firstHalf == true) ? (0) : (ADC_BUFFER_SIZE / 2);

  if (feedback_buffer_index >= PROCESSING_BUFFER_SIZE) {
    feedback_buffer_index = feedback_buffer_index % PROCESSING_BUFFER_SIZE;
  }

  if (feedback_buffer_index + ADC_BUFFER_SIZE > PROCESSING_BUFFER_SIZE) {
    uint16_t first_block_size = PROCESSING_BUFFER_SIZE - feedback_buffer_index;
    memcpy(&feedback_buffer[feedback_buffer_index], &adc_buffer[dma_buf_start_index], first_block_size * sizeof(uint16_t));
    uint16_t second_block_size = ADC_BUFFER_SIZE / 2 - first_block_size;
    memcpy(&feedback_buffer[feedback_buffer_index], &adc_buffer[first_block_size + dma_buf_start_index], second_block_size * sizeof(uint16_t));
  }
  else {
    memcpy(&feedback_buffer[feedback_buffer_index], &adc_buffer[dma_buf_start_index], (ADC_BUFFER_SIZE / 2) * sizeof(uint16_t));
  }

  feedback_buffer_index = (feedback_buffer_index + ADC_BUFFER_SIZE / 2) % PROCESSING_BUFFER_SIZE;

  Feedback_IncrementEndIndex();
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
