/*
 * ina219-driver.c
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "cmsis_os.h"
#include "ina219-driver.h"
#include "error_manager.h"
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  INA_IDLE,
  INA_READING_SHUNT_VOLTAGE,
  INA_READING_BUS_VOLTAGE,
  INA_ERROR
} InaState_t;

// Configuration Bits for INA219:

// Reset bit (Bit 15): Resets all registers to default values. Self clearing
typedef enum {
  INA_RESET = 0x8000,  // Reset
  INA_NORMAL = 0x0000, // Normal operation
} InaReset_t; 

// Bus voltage range options (Bit 13) 16V or 32V FSR
typedef enum {
  INA_RANGE_16V = 0x0000, // 16V FSR
  INA_RANGE_32V = 0x2000, // 32V FSR (DEFAULT)
} InaBusVoltageRange;

// PGA gain and range (Bits 11-9): /8 gain and 320mV range is default
typedef enum {
  INA_GAIN_1_40MV =  0x0000, // 1 gain, 40mV range
  INA_GAIN_2_80MV =  0x0800, // /2 gain, 80mV range
  INA_GAIN_4_160MV = 0x1000, // /4 gain, 160mV range
  INA_GAIN_8_320MV = 0x1800, // /8 gain, 320mV range (DEFAULT)
} InaGain_t;

// Bus ADC Resolution and Averaging (Bits 10-7): 12 bit, 1 sample is default
typedef enum {
  INA_BUS_ADC_9BIT =        0x0000, // 9-bit, 1 sample
  INA_BUS_ADC_10BIT =       0x0080, // 10-bit, 1 sample
  INA_BUS_ADC_11BIT =       0x0100, // 11-bit, 1 sample
  INA_BUS_ADC_12BIT =       0x0180, // 12-bit, 1 sample (DEFAULT)
  INA_BUS_ADC_12BIT_2S =    0x0480, // 12-bit, 2 samples when averaging results
  INA_BUS_ADC_12BIT_4S =    0x0500, // 12-bit, 4 samples when averaging results
  INA_BUS_ADC_12BIT_8S =    0x0580, // 12-bit, 8 samples when averaging results
  INA_BUS_ADC_12BIT_16S =   0x0600, // 12-bit, 16 samples when averaging results
  INA_BUS_ADC_12BIT_32S =   0x0680, // 12-bit, 32 samples when averaging results
  INA_BUS_ADC_12BIT_64S =   0x0700, // 12-bit, 64 samples when averaging results
  INA_BUS_ADC_12BIT_128S =  0x0780, // 12-bit, 128 samples when averaging results
} InaBusAdc_t;

// Shunt ADC Resolution and Averaging (Bits 6-3): 12 bit, 1 sample is default
typedef enum {
  INA_SHUNT_ADC_9BIT =        0x0000,
  INA_SHUNT_ADC_10BIT =       0x0008,
  INA_SHUNT_ADC_11BIT =       0x0010,
  INA_SHUNT_ADC_12BIT =       0x0018, // (DEFAULT)
  INA_SHUNT_ADC_12BIT_2S =    0x0048, // 12-bit, 2 samples when averaging results
  INA_SHUNT_ADC_12BIT_4S =    0x0050, // 12-bit, 4 samples when averaging results
  INA_SHUNT_ADC_12BIT_8S =    0x0058, // 12-bit, 8 samples when averaging results
  INA_SHUNT_ADC_12BIT_16S =   0x0060, // 12-bit, 16 samples when averaging results
  INA_SHUNT_ADC_12BIT_32S =   0x0068, // 12-bit, 32 samples when averaging results
  INA_SHUNT_ADC_12BIT_64S =   0x0070, // 12-bit, 64 samples when averaging results
  INA_SHUNT_ADC_12BIT_128S =  0x0078, // 12-bit, 128 samples when averaging results
} InaShuntAdc_t;


// Operating modes (Bits 2-0): Shunt and bus voltage continuous is default
typedef enum {
  INA_MODE_POWER_DOWN =                       0x0000,
  INA_MODE_SHUNT_VOLTAGE_TRIGGERED =          0x0001, // Shunt voltage triggered
  INA_MODE_BUS_VOLTAGE_TRIGGERED =            0x0002, // Bus voltage triggered
  INA_MODE_SHUNT_AND_BUS_VOLTAGE_TRIGGERED =  0x0003, // Shunt and bus voltage triggered
  INA_MODE_ADC_OFF =                          0x0004, // ADC off
  INA_MODE_SHUNT_VOLTAGE_CONTINUOUS =         0x0005, // Shunt voltage continuous
  INA_MODE_BUS_VOLTAGE_CONTINUOUS =           0x0006, // Bus voltage continuous
  INA_MODE_SHUNT_AND_BUS_VOLTAGE_CONTINUOUS = 0x0007, // Shunt and bus voltage continuous (DEFAULT)
} InaMode_t;

typedef struct {
  InaPowerValues_t* buf;
  uint16_t buf_len;
  volatile uint16_t* buf_head;
} PowerBufferInfo_t;

/* Private define ------------------------------------------------------------*/

#define CONFIG_VOLTAGE_RANGE        INA_RANGE_32V
#define CONFIG_GAIN                 INA_GAIN_8_320MV // No precision gained by higher gain
#define CONFIG_SHUNT_ADC            INA_SHUNT_ADC_12BIT
#define CONFIG_BUS_ADC              INA_BUS_ADC_12BIT
#define CONFIG_MODE                 INA_MODE_SHUNT_AND_BUS_VOLTAGE_CONTINUOUS

#define INA_I2C_BUS                 hi2c1

// Values
#define SHUNT_RESISTOR_VALUE        0.01f // 10mOhm shunt resistor

// INA219 Register Addresses
#define CONFIGURATION_ADDRESS       0x00
#define SHUNT_VOLTAGE_ADDRESS       0x01
#define BUS_VOLTAGE_ADDRESS         0x02
#define POWER_ADDRESS               0x03
#define CURRENT_ADDRESS             0x04
#define CALIBRATION_ADDRESS         0x05

#define INA219_ADDRESS              0x40    // Device base address
#define BUS_MV_PER_LSB              4
#define SHUNT_UV_PER_LSB            10

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

static InaState_t ina_state = INA_IDLE;
static PowerBufferInfo_t power_buffer_info = {NULL, 0, NULL};

static float shunt_conversion_factor = (SHUNT_UV_PER_LSB / 1000000.0f) / SHUNT_RESISTOR_VALUE; 

static volatile uint16_t tx_data;
static volatile uint16_t rx_data;

static volatile bool ina_ready = false;

extern I2C_HandleTypeDef hi2c1; // I2C handle for communication

/* Private function prototypes -----------------------------------------------*/

static bool memWrite(uint16_t address, uint16_t data);
static bool memRead(uint16_t address);

static void readIna(void);

static float convertRawShuntVoltage(uint16_t raw_reading);
static float convertRawBusVoltage(uint16_t raw_reading);

/* Exported functions Definitions ---------------------------------------------*/

void INA_Init() // Set configuration register
{
  ina_state = INA_IDLE;
  ina_ready = false;

  uint16_t config = CONFIG_VOLTAGE_RANGE |
                    CONFIG_GAIN |
                    CONFIG_SHUNT_ADC |
                    CONFIG_BUS_ADC |
                    CONFIG_MODE;

  if (memWrite(CONFIGURATION_ADDRESS, config) == false) 
    REGISTER_ERROR(ERROR_POWER_MONITOR);
  
  if (ina_ready == false) REGISTER_ERROR(ERROR_POWER_MONITOR); 
}

void INA_RegisterBuffer(InaPowerValues_t* buf, uint16_t buf_len, volatile uint16_t* buf_head)
{
  if (buf == NULL || buf_len == 0 || buf_head == NULL) REGISTER_ERROR(ERROR_NULL_PTR);

  power_buffer_info.buf = buf;
  power_buffer_info.buf_len = buf_len;
  power_buffer_info.buf_head = buf_head;
}

void INA_StartRead(void)
{
  if ((power_buffer_info.buf_head == NULL) || (power_buffer_info.buf == NULL)) 
    REGISTER_ERROR(ERROR_POWER_MONITOR);

  if (ina_state != INA_IDLE) return;
  if (ina_ready == false) return;

  readIna();
}

void INA_RxComplete(void)
{
  if (ina_state != INA_IDLE) readIna();
}

void INA_TxComplete(void)
{
  ina_ready = true;
}

/* Private function definitions ----------------------------------------------*/

bool memWrite(uint16_t address, uint16_t data)
{
  tx_data = __REV16(data);
  if (HAL_I2C_Mem_Write_IT(&INA_I2C_BUS, INA219_ADDRESS << 1, address, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &tx_data, 2) != HAL_OK) {
    return false;
  }
  osDelay(1); // Small delay to ensure transaction goes through. Flags not used since only one register written to
  return true;
}

bool memRead(uint16_t address)
{
  if (HAL_I2C_Mem_Read_IT(&INA_I2C_BUS, INA219_ADDRESS << 1, address, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &rx_data, 2) != HAL_OK) {
    return false;
  }
  return true;
}

void readIna(void)
{
  InaPowerValues_t* entry = &power_buffer_info.buf[*power_buffer_info.buf_head];
  switch (ina_state) {
    case INA_IDLE:
      if (memRead(SHUNT_VOLTAGE_ADDRESS) == false) 
        REGISTER_ERROR(ERROR_POWER_MONITOR);
      ina_state = INA_READING_SHUNT_VOLTAGE;
      break;
    case INA_READING_SHUNT_VOLTAGE:
      entry->current_A = convertRawShuntVoltage(__REV16(rx_data));
      if (memRead(BUS_VOLTAGE_ADDRESS) == false) 
        REGISTER_ERROR(ERROR_POWER_MONITOR);
      ina_state = INA_READING_BUS_VOLTAGE;
      break;
    case INA_READING_BUS_VOLTAGE:
      entry->bus_voltage = convertRawBusVoltage(__REV16(rx_data));
      entry->power = entry->bus_voltage * entry->current_A;
      entry->timestamp = HAL_GetTick();
      *power_buffer_info.buf_head = (*power_buffer_info.buf_head + 1) % power_buffer_info.buf_len;
      ina_state = INA_IDLE;
      break;
    case INA_ERROR:
      REGISTER_ERROR(ERROR_POWER_MONITOR);
      break;
    default:
      REGISTER_ERROR(ERROR_POWER_MONITOR);
  }
}

// Converts to current
float convertRawShuntVoltage(uint16_t raw_reading)
{
  return ((float) raw_reading) * shunt_conversion_factor;
}

float convertRawBusVoltage(uint16_t raw_reading)
{
  return ((float) ((raw_reading >> 3) * BUS_MV_PER_LSB)) * 0.001f;
}
