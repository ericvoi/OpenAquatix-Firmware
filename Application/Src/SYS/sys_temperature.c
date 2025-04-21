/*
 * sys_temperature.c
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"

#include "sys_temperature.h"

#include "mess_adc.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  uint16_t raw_value;
  float converted_value_c;
} Temperature_t;

/* Private define ------------------------------------------------------------*/

#define TS_CAL1_TEMP              (30.0f)
#define TS_CAL2_TEMP              (130.0f)

#define TS_CAL1_ADDRESS           0x1FF1E820 // Stores calibration value at 30C
#define TS_CAL2_ADDRESS           0x1FF1E840 // Stores calibration value at 130C

#define TEMPERATURE_BUFFER_SIZE   8

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t ts_cal1;
static uint16_t ts_cal2;

static float leading_factor; // constant multiplicative factor when converting adc values to temperature

static Temperature_t temperature_buffer[TEMPERATURE_BUFFER_SIZE];

static uint16_t buf_index = 0; // Where new data should go
static uint16_t processing_index = 0; // Where to start processing data from

static float max_temperature = -1000.0f;

static uint64_t temperature_count = 0;
static uint64_t accumulated_raw_adc = 0;


/* Private function prototypes -----------------------------------------------*/

float rawToTemperature(uint16_t raw);

/* Exported function definitions ---------------------------------------------*/

bool Temperature_Init()
{
  // Read factory calibration data
  ts_cal1 = *((uint16_t*) TS_CAL1_ADDRESS);
  ts_cal2 = *((uint16_t*) TS_CAL2_ADDRESS);

  leading_factor = (TS_CAL2_TEMP - TS_CAL1_TEMP) / ((float) ts_cal2 - ts_cal1);

  return true;
}

bool Temperature_TriggerConversion()
{
  if (HAL_ADC_Start_IT(&TEMPERATURE_ADC) != HAL_OK) {
    return false;
  }

  return true;
}

void Temperature_AddValue()
{
  uint16_t adc_value = HAL_ADC_GetValue(&TEMPERATURE_ADC);

  HAL_ADC_Stop_IT(&TEMPERATURE_ADC);

  temperature_buffer[buf_index].raw_value = adc_value;
  temperature_buffer[buf_index].converted_value_c = rawToTemperature(adc_value);
  buf_index = (buf_index + 1) % TEMPERATURE_BUFFER_SIZE;
}

bool Temperature_Process()
{
  if (processing_index == buf_index) {
    return true;
  }

  uint16_t remaining_length = (buf_index - processing_index - 1) % TEMPERATURE_BUFFER_SIZE;

  for (uint16_t i = 0; i < remaining_length; i++) {
    uint16_t index = (processing_index + i) % TEMPERATURE_BUFFER_SIZE;
    accumulated_raw_adc += temperature_buffer[index].raw_value;
    temperature_count++;

    if (temperature_buffer[index].converted_value_c > max_temperature) {
      max_temperature = temperature_buffer[index].converted_value_c;
    }
    processing_index = (processing_index + 1) % TEMPERATURE_BUFFER_SIZE;
  }
  return true;
}

float Temperature_GetAverage()
{
  uint16_t raw_average = accumulated_raw_adc / temperature_count;

  return rawToTemperature(raw_average);
}

float Temperature_GetCurrent()
{
  uint16_t current_index = (buf_index - 1) % TEMPERATURE_BUFFER_SIZE;

  return temperature_buffer[current_index].converted_value_c;
}

float Temperature_GetPeak()
{
  return max_temperature;
}

/* Private function definitions ----------------------------------------------*/

float rawToTemperature(uint16_t raw)
{
  return leading_factor * ((float) raw - ts_cal1) + 30.0f;
}
