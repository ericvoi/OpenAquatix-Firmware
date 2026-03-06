/*
 * sys_power.c
 *
 *  Created on: Jan 31, 2025
 *      Author: cjcockrall
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */


/* Private includes ----------------------------------------------------------*/
#include "ina219-driver.h"
#include "sys_power.h"
#include "error_manager.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define POWER_BUFFER_SIZE       64 // Size of the power buffer for averaging

#define POWER_QUANTIZATION      1000
#define CURRENT_QUANTIZATION    10000
#define VOLTAGE_QUANTIZATION    1000

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static InaPowerValues_t power_buffer[POWER_BUFFER_SIZE]; // Buffer to hold power readings and timestamps

static volatile uint16_t buffer_head = 0;
static uint16_t buffer_tail = 0;

static float min_power = 1e7;
static float max_power = 0;
static float min_current = 1e7;
static float max_current = 0;
static float min_voltage = 1e7;
static float max_voltage = 0;

/*
 * Given a target average period of 6 months and 1ms sample intervals, this
 * would result in 1.5E10 samples total. Max uint32_t is 4.3E9 and max uint64_t
 * is 1.8E19. Therefore, sample count and accumulation variables must be
 * uint64_t. With uint64_t the max quantized size is 1.2E9, so the 12-bit 
 * resolution of the INA is decidedly the limiting factor 
 */
static uint64_t acc_sum_power = 0;   // 1 mW precision
static uint64_t acc_sum_current = 0; // 100 uA precision
static uint64_t acc_sum_voltage = 0; // 1 mV precision
static uint64_t acc_count = 0;       // Number of values in average

/* External variables --------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/
void Power_Init(void)
{
  RETURN_IF_ERROR_PRESENT();
  INA_Init();

  // Clear the power buffer for fresh readings
  memset(power_buffer, 0, sizeof(power_buffer));
  buffer_head = 0;

  INA_RegisterBuffer(power_buffer, POWER_BUFFER_SIZE, &buffer_head);
}

void Power_Process(void)
{
  while (buffer_tail != buffer_head) {
    float power = power_buffer[buffer_tail].power;
    float current = power_buffer[buffer_tail].current_A;
    float voltage = power_buffer[buffer_tail].bus_voltage;
    acc_sum_power   += (uint64_t) (power   * POWER_QUANTIZATION);
    acc_sum_current += (uint64_t) (current * CURRENT_QUANTIZATION);
    acc_sum_voltage += (uint64_t) (voltage * VOLTAGE_QUANTIZATION);
    acc_count += 1;

    if (power > max_power) max_power = power;
    if (power < min_power) min_power = power;

    if (current > max_current) max_current = current;
    if (current < min_current) min_current = current;

    if (voltage > max_voltage) max_voltage = voltage;
    if (voltage < min_voltage) min_voltage = voltage;

    buffer_tail = (buffer_tail + 1) % POWER_BUFFER_SIZE;
  }
}

float Power_MinPower(void)
{
  return min_power;
}

float Power_MaxPower(void)
{
  return max_power;
}

float Power_AveragePower(void)
{
  return ((float) (acc_sum_power / acc_count)) / ((float) POWER_QUANTIZATION);
}

// Calculates the recent average of power readings, up to the size of the buffer
float Power_RecentAveragePower(uint8_t samples)
{
  if (samples == 0 || samples >= POWER_BUFFER_SIZE) {
    return 0.0f; // Invalid sample size
  }

  float sum = 0.0f;
  uint8_t count = 0;

  uint16_t head = buffer_head; // Snapshot incase new sample is added
  
  for (uint16_t i = 1; i < samples - 1; i++) {
    uint16_t index = ((uint16_t) (head - i)) % POWER_BUFFER_SIZE;
    if (power_buffer[index].timestamp != 0) {
      sum += power_buffer[index].power;
      count++;
    }
  }
  // Return the average in watts as long as there is at least one valid reading. Otherwise, return 0.
  return (count > 0) ? (sum / count) : 0.0f; 
}

float Power_LatestPower(void)
{
  uint16_t index = ((uint16_t) (buffer_head - 1)) % POWER_BUFFER_SIZE;
  return power_buffer[index].power;
}

float Power_MinVoltage(void)
{
  return min_voltage;
}

float Power_MaxVoltage(void)
{
  return max_voltage;
}

float Power_AverageVoltage(void)
{
  return ((float) (acc_sum_voltage / acc_count)) / ((float) VOLTAGE_QUANTIZATION);
}

float Power_LatestVoltage(void)
{
  uint16_t index = ((uint16_t) (buffer_head - 1)) % POWER_BUFFER_SIZE;
  return power_buffer[index].bus_voltage;
}

float Power_MinCurrent(void)
{
  return min_current;
}

float Power_MaxCurrent(void)
{
  return max_current;
}

float Power_AverageCurrent(void)
{
  return ((float) (acc_sum_current / acc_count)) / ((float) CURRENT_QUANTIZATION);
}

float Power_LatestCurrent(void)
{
  uint16_t index = ((uint16_t) (buffer_head - 1)) % POWER_BUFFER_SIZE;
  return power_buffer[index].current_A;
}

/* Private function definitions ----------------------------------------------*/
