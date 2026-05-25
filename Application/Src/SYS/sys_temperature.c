/*
 * sys_temperature.c
 *
 *  Created on: Apr 20, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "sys_temperature.h"
#include "mess_filt_resources.h" // TODO: change ADC dependency to be out of here
#include "lps22hh-driver.h"
#include "error_manager.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  int16_t raw_value;
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

/* Junction Temperature Variables ============================================*/

// Calibration factors loaded from flash during intialization
static uint16_t ts_cal1;
static uint16_t ts_cal2;

// Constant multiplicative factor when converting adc values to temperature
static float tj_leading_factor;

// Stores latest temperature values both raw and converted
static Temperature_t tj_buf[TEMPERATURE_BUFFER_SIZE];

static uint16_t tj_buf_head = 0; // Where new data should go
static uint16_t tj_buf_tail = 0; // Where to start processing data from

static float current_tj;
static float tj_max = -1000.0f; // -1000 So the first value always overwrites

// The number of temperature readings that have been taken
static uint64_t tj_count = 0;
// Sum of all raw adc temperature values
static uint64_t accumulated_raw_tj = 0;

extern ADC_HandleTypeDef hadc3;

/* Ambient Temperature Variables ============================================*/

static int16_t ta_buf[TEMPERATURE_BUFFER_SIZE];
static uint16_t ta_buf_head = 0;
static uint16_t ta_buf_tail = 0;

static float current_ta;
static float ta_max = -1000.0f;

static uint64_t ta_count = 0;
static int64_t accumulated_raw_ta = 0;

/* Private function prototypes -----------------------------------------------*/

float rawTjToTemperature(uint16_t raw);

/* Exported function definitions ---------------------------------------------*/

void Temperature_Init()
{
  RETURN_IF_ERROR_PRESENT();
  // Read factory calibration data
  ts_cal1 = *((uint16_t*) TS_CAL1_ADDRESS);
  ts_cal2 = *((uint16_t*) TS_CAL2_ADDRESS);

  tj_leading_factor = (TS_CAL2_TEMP - TS_CAL1_TEMP) / ((float) ts_cal2 - ts_cal1);

  ta_buf_head = 0;
  ta_buf_tail = 0;
  LPS_RegisterTemperatureBuf(ta_buf, TEMPERATURE_BUFFER_SIZE, &ta_buf_head);
}

void Temperature_TriggerTjConversion()
{
  if (HAL_ADC_Start_IT(&TEMPERATURE_ADC) != HAL_OK) 
    REGISTER_ERROR(ERROR_JUNCTION_TEMPERATURE);
}

void Temperature_AddTjValue()
{
  uint16_t adc_value = HAL_ADC_GetValue(&TEMPERATURE_ADC);

  HAL_ADC_Stop_IT(&TEMPERATURE_ADC);

  tj_buf[tj_buf_head].raw_value = adc_value;
  tj_buf[tj_buf_head].converted_value_c = rawTjToTemperature(adc_value);
  tj_buf_head = (tj_buf_head + 1) % TEMPERATURE_BUFFER_SIZE;
}

void Temperature_Process()
{
  while (tj_buf_head != tj_buf_tail) {
    accumulated_raw_tj += tj_buf[tj_buf_tail].raw_value;
    tj_count++;

    current_tj = tj_buf[tj_buf_tail].converted_value_c;
    if (tj_buf[tj_buf_tail].converted_value_c > tj_max) {
      tj_max = tj_buf[tj_buf_tail].converted_value_c;
    }
    tj_buf_tail = (tj_buf_tail + 1) % TEMPERATURE_BUFFER_SIZE;
  }

  while (ta_buf_head != ta_buf_tail) {
    accumulated_raw_ta += ta_buf[ta_buf_tail];
    tj_count++;

    float val_c = LPS_ConvertRawTemperature(ta_buf[ta_buf_tail]);
    current_ta = val_c;
    if (val_c > ta_max) {
      ta_max = val_c;
    }
    ta_buf_tail = (ta_buf_tail + 1) % TEMPERATURE_BUFFER_SIZE;
  }
}

float Temperature_GetAverageTj()
{
  uint16_t raw_average = accumulated_raw_tj / tj_count;

  return rawTjToTemperature(raw_average);
}

float Temperature_GetCurrentTj()
{
  return current_tj;
}

float Temperature_GetPeakTj()
{
  return tj_max;
}

float Temperature_GetAverageTa(void)
{
  int16_t raw_average = accumulated_raw_ta / ta_count;

  return LPS_ConvertRawTemperature(raw_average);
}

float Temperature_GetCurrentTa(void)
{
  return current_ta;
}

float Temperature_GetPeakTa(void)
{
  return ta_max;
}

/* Private function definitions ----------------------------------------------*/

float rawTjToTemperature(uint16_t raw)
{
  return tj_leading_factor * ((float) raw - ts_cal1) + 30.0f;
}
