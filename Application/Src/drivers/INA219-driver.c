/*
 * INA219-driver.c
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "INA219-driver.h"
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

// Configuration Bits for INA219:

// Reset bit (Bit 15): Resets all registers to default values. Self clearing
typedef enum
{
    INA_RESET = 0x8000,  // Reset
    INA_NORMAL = 0x0000, // Normal operation
} INA219_Reset_t; 

// Bus voltage range options (Bit 13) 16V or 32V FSR
typedef enum
{
    INA_RANGE_16V = 0x0000, // 16V FSR
    INA_RANGE_32V = 0x2000, // 32V FSR (DEFAULT)
} INA219_BusVoltageRange_t;

// PGA gain and range (Bits 11-9): /8 gain and 320mV range is default
typedef enum
{
    INA_GAIN_1_40MV = 0x0000,  // 1 gain, 40mV range
    INA_GAIN_2_80MV = 0x0800,  // /2 gain, 80mV range
    INA_GAIN_4_160MV = 0x1000, // /4 gain, 160mV range
    INA_GAIN_8_320MV = 0x1800, // /8 gain, 320mV range (DEFAULT)
} INA219_Gain_t;

// Bus ADC Resolution and Averaging (Bits 10-7): 12 bit, 1 sample is default
typedef enum
{
    INA_SHUNT_ADC_9BIT = 0x0000,       // 9-bit, 1 sample
    INA_SHUNT_ADC_10BIT = 0x0200,      // 10-bit, 1 sample
    INA_SHUNT_ADC_11BIT = 0x0400,      // 11-bit, 1 sample
    INA_SHUNT_ADC_12BIT = 0x0600,      // 12-bit, 1 sample (DEFAULT)
    INA_SHUNT_ADC_12BIT_2S = 0x0800,   // 12-bit, 2 samples when averaging results
    INA_SHUNT_ADC_12BIT_4S = 0x0A00,   // 12-bit, 4 samples when averaging results
    INA_SHUNT_ADC_12BIT_8S = 0x0C00,   // 12-bit, 8 samples when averaging results
    INA_SHUNT_ADC_12BIT_16S = 0x0E00,  // 12-bit, 16 samples when averaging results
    INA_SHUNT_ADC_12BIT_32S = 0x1000,  // 12-bit, 32 samples when averaging results
    INA_SHUNT_ADC_12BIT_64S = 0x1200,  // 12-bit, 64 samples when averaging results
    INA_SHUNT_ADC_12BIT_128S = 0x1400, // 12-bit, 128 samples when averaging results
} INA219_BusADC_t;

// Shunt ADC Resolution and Averaging (Bits 6-3): 12 bit, 1 sample is default
typedef enum
{
    INA_BUS_ADC_9BIT = 0x0000,
    INA_BUS_ADC_10BIT = 0x0008,
    INA_BUS_ADC_11BIT = 0x0010,
    INA_BUS_ADC_12BIT = 0x0018,      // (DEFAULT)
    INA_BUS_ADC_12BIT_2S = 0x0020,   // 12-bit, 2 samples when averaging results
    INA_BUS_ADC_12BIT_4S = 0x0028,   // 12-bit, 4 samples when averaging results
    INA_BUS_ADC_12BIT_8S = 0x0030,   // 12-bit, 8 samples when averaging results
    INA_BUS_ADC_12BIT_16S = 0x0038,  // 12-bit, 16 samples when averaging results
    INA_BUS_ADC_12BIT_32S = 0x0040,  // 12-bit, 32 samples when averaging results
    INA_BUS_ADC_12BIT_64S = 0x0048,  // 12-bit, 64 samples when averaging results
    INA_BUS_ADC_12BIT_128S = 0x0050, // 12-bit, 128 samples when averaging results
} INA219_ShuntADC_t;


// Operating modes (Bits 2-0): Shunt and bus voltage continuous is default
typedef enum
{
    INA_MODE_POWER_DOWN = 0x0000,
    INA_MODE_SHUNT_VOLTAGE_TRIGGERED = 0x0001,          // Shunt voltage triggered
    INA_MODE_BUS_VOLTAGE_TRIGGERED = 0x0002,            // Bus voltage triggered
    INA_MODE_SHUNT_AND_BUS_VOLTAGE_TRIGGERED = 0x0003,  // Shunt and bus voltage triggered
    INA_MODE_ADC_OFF = 0x0004,                          // ADC off
    INA_MODE_SHUNT_VOLTAGE_CONTINUOUS = 0x0005,         // Shunt voltage continuous
    INA_MODE_BUS_VOLTAGE_CONTINUOUS = 0x0006,           // Bus voltage continuous
    INA_MODE_SHUNT_AND_BUS_VOLTAGE_CONTINUOUS = 0x0007, // Shunt and bus voltage continuous (DEFAULT)

} INA219_Mode_t;

/* Private define ------------------------------------------------------------*/

// Values
#define INA219_SHUNT_RESISTOR_VALUE 0.01f // 10mOhm shunt resistor
// #define INA219_CURRENT_LSB 0.00001f // 10uA per LSB (may adjust)
#define INA219_MAX_EXPECTED_CURRENT 3.0f // Maximum expected current in Amps (need adjust)

// INA219 Register Addresses
#define CONFIGURATION_ADDRESS 0x00
#define SHUNT_VOLTAGE_ADDRESS 0x01
#define BUS_VOLTAGE_ADDRESS 0x02
#define POWER_ADDRESS 0x03
#define CURRENT_ADDRESS 0x04
#define CALIBRATION_ADDRESS 0x05

#define INA219_ADDRESS 0x40 // I2C address of INA219 (10000000)

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

// Status structure to track state of INA219, initialized to initialization state 0
static struct
{
    volatile uint8_t init_state;
    volatile bool transfer_complete;
    volatile bool transfer_error;
} ina219_status = {
    .init_state = 0,
    .transfer_complete = false,
    .transfer_error = false};


// Current and power calibration value, bits [15:1] are used. See equation 1 in INA219 Datasheet
uint16_t cal_value = (uint16_t)(0.04096/(INA219_CURRENT_LSB*INA219_SHUNT_RESISTOR_VALUE)); 

uint16_t reset_cmd = INA_RESET; // Configuration bits for reset, used before initialization

extern I2C_HandleTypeDef hi2c1; // I2C handle for communication

/* Private function prototypes -----------------------------------------------*/

/**
 * @brief Callback function for I2C memory transfer completion
 *
 * This function is called when an I2C memory transfer is completed.
 * It sets the transfer_complete flag to true.
 *
 * @param hi2c Pointer to the I2C handle
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);


/**
 * @brief Callback function for I2C error handling
 *
 * This function is called when an I2C error occurs.
 * It sets the transfer_error flag to true.
 *
 * @param hi2c Pointer to the I2C handle
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);

/* Exported functions Definitions ---------------------------------------------*/
// Function to initialize INA219 (non-blocking using DMA and callbacks)
HAL_StatusTypeDef INA219_Init() // Set configuration register and calibration register
{
    switch (ina219_status.init_state) { 
        case 0: // Start reset
            ina219_status.transfer_complete = false;
            if (HAL_I2C_Mem_Write_DMA(&hi2c1,
                                        INA219_ADDRESS << 1,
                                        CONFIGURATION_ADDRESS,
                                        I2C_MEMADD_SIZE_8BIT,
                                        (uint8_t*)&reset_cmd,
                                        sizeof(reset_cmd)) != HAL_OK) {
                return HAL_ERROR;
            }
            ina219_status.init_state = 1; // Proceed to next state
            return HAL_BUSY;

        case 1: // Wait for reset complete
            if (!ina219_status.transfer_complete) {
                return HAL_BUSY;
            }
            if (ina219_status.transfer_error) {
                ina219_status.init_state = 0;
                return HAL_ERROR;
            }
            HAL_Delay(1); 
            ina219_status.init_state = 2; // Proceed to next state
            return HAL_BUSY;

        case 2: // Set configuration register based on config_value
            ina219_status.transfer_complete = false;
            uint16_t config_value = INA_NORMAL | 
                                    INA_RANGE_32V | 
                                    INA_GAIN_8_320MV |
                                    INA_SHUNT_ADC_12BIT | 
                                    INA_BUS_ADC_12BIT |
                                    INA_MODE_SHUNT_AND_BUS_VOLTAGE_CONTINUOUS;

            if (HAL_I2C_Mem_Write_DMA(&hi2c1,
                                        INA219_ADDRESS << 1,
                                        CONFIGURATION_ADDRESS,
                                        I2C_MEMADD_SIZE_8BIT,
                                        (uint8_t*)&config_value,
                                        sizeof(config_value)) != HAL_OK) {
                return HAL_ERROR;
            }
            ina219_status.init_state = 3; // Proceed to next state
            return HAL_BUSY;

        case 3: // Wait for configuration complete
            if (!ina219_status.transfer_complete) {
                return HAL_BUSY;
            }
            if (ina219_status.transfer_error) {
                ina219_status.init_state = 0;
                return HAL_ERROR;
            }
            ina219_status.init_state = 4; // Proceed to next state
            return HAL_OK;

        case 4: // Set calibration register
            ina219_status.transfer_complete = false;
            if (HAL_I2C_Mem_Write_DMA(&hi2c1,
                                      INA219_ADDRESS << 1,
                                      CALIBRATION_ADDRESS,
                                      I2C_MEMADD_SIZE_8BIT,
                                      (uint8_t*)&cal_value,
                                      sizeof(cal_value)) != HAL_OK) {
                return HAL_ERROR;
            }
            ina219_status.init_state = 5; // Proceed to next state
            return HAL_BUSY;

        case 5: // Wait for calibration complete
            if (!ina219_status.transfer_complete) {
                return HAL_BUSY;
            }
            if (ina219_status.transfer_error) {
                ina219_status.init_state = 0;
                return HAL_ERROR;
            }
            ina219_status.init_state = 0; // Reset state for next initialization
            return HAL_OK;

        default:
            ina219_status.init_state = 0; // Reset state on unexpected value
            return HAL_ERROR;
        }
}

// DMA callback for transfer complete
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == hi2c1.Instance) {
        ina219_status.transfer_complete = true;
        ina219_status.transfer_error = false;
    }
}

// DMA callback for transfer error
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == hi2c1.Instance) {
        ina219_status.transfer_complete = false;
        ina219_status.transfer_error = true;
    }
}