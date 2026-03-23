/*
 * lps22hh-driver.h
 *
 *  Created on: Jan 1, 2026
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef DRIVERS_LPS22HH_DRIVER_H_
#define DRIVERS_LPS22HH_DRIVER_H_

/**
 * Driver written only with the required use case of the modem in mind. Data
 * collection and alarms are done manually for ease of debugging
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  LPS_ODR_1   = 1,
  LPS_ODR_10  = 2,
  LPS_ODR_25  = 3,
  LPS_OSR_50  = 4,
  LPS_ODR_75  = 5,
  LPS_ODR_100 = 6,
  LPS_ODR_200 = 7
} LpsOdr_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/

#define LPS22HH_SPI_BUS         hspi3

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes mutex and event resources for LPS driver
 * 
 * @note Only call this function once
 */
void LPS_CreateResources(void);

/**
 * @brief Initializes registers in LPA22HH, creates mutex, and events
 * 
 * @param odr Rate at which pressure/temperature samples should be created
 */
void LPS_Init(LpsOdr_t odr);

/**
 * @brief Registers a buffer to store raw pressure values
 * 
 * @param p_buf Array to store raw pressure readings
 * @param buf_len Length of p_buf
 * @param buf_head Pointer to head of array (modified when new sample added)
 * 
 * @note If this function is not called, pressure readings will not be read
 */
void LPS_RegisterPressureBuf(uint32_t* p_buf, uint16_t buf_len, uint16_t* buf_head);

/**
 * @brief Registers a buffer to store raw pressure values
 * 
 * @param t_buf Array to store raw temperature readings
 * @param buf_len Length of t_buf
 * @param buf_head Pointer to head of array (modified when new sample added)
 * 
 * @note If this function is not called, temperature readings will not be read
 */
void LPS_RegisterTemperatureBuf(int16_t* t_buf, uint16_t buf_len, uint16_t* buf_head);

/**
 * @brief Powers down the LPS22HH to reduce power consumption
 */
void LPS_PowerDown(void);

/**
 * @brief Powers up the LPS22HH to get readings after device powered down
 * 
 * @note uses the same ODR as when the device was last booted
 */
void LPS_PowerUp(void);

/**
 * @brief Attempts to read the WHO_AMI_I register to check if the SPI interface
 * and digital frontend of the LPS22HH is working
 * 
 * @return true if successfully read register and contents of WHO_AM_I match
 * expected
 */
bool LPS_CheckInterface(void);

/**
 * @brief Reads the pressure and temperature data registers of the device if
 * a buffer has been registered for them and adds readings to buffers
 */
void LPS_ReadData(void);

/**
 * @brief Converts a pressure reading from the LPS22HH to a floating point
 * value with units hPa (milli-bar)
 * 
 * @param raw_reading Raw digital pressure data obtained from LPS22HH
 * @return float Pressure in hPa
 */
float LPS_ConvertRawPressure(uint32_t raw_reading);

/**
 * @brief Converts a temperature reading from the LPS22HH to a floating point
 * value with units Celsius
 * 
 * @param raw_reading Raw digital temperature data obtained from LPS22HH
 * @return float Temperature in Celsius
 */
float LPS_ConvertRawTemperature(int16_t raw_reading);

// SPI callbacks

/**
 * @brief Call from interrupt when SPI TX finished.
 * 
 * Informs driver that the transaction is complete
 */
void LPS_TxComplete(void);

/**
 * @brief Call from interrupt when SPI RX finished.
 * 
 * Informs driver that the transaction is complete
 */
void LPS_RxComplete(void);

/**
 * @brief Call from interrupt when the LPS22HH asserts the INT_DRDY pin
 */
void LPS_IntDrdy(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_LPS22HH_DRIVER_H_ */
