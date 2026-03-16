/*
 * lps22hh-driver.c
 *
 *  Created on: Jan 1, 2026
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "lps22hh-driver.h"
#include "cmsis_os.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  LPS_SPI_CLEAR = 1 << 0,
  LPS_DATA_READY = 1 << 1
} LpsEvents_t;

typedef union {
  struct {
    uint8_t blank;
    uint8_t h;
    uint8_t l;
    uint8_t xl;
  } registers;
  uint32_t raw_pressure;
} RawPressureData_t;

typedef union {
  struct {
    uint8_t h;
    uint8_t l;
  } registers;
  uint16_t raw_temperature;
} RawTemperatureData_t;

typedef struct {
  RawPressureData_t* buf;
  uint16_t len;
  uint16_t* head;
} PressureBufferInfo_t;

typedef struct {
  RawTemperatureData_t* buf;
  uint16_t len;
  uint16_t* head;
} TemperatureBufferInfo_t;

/* Private define ------------------------------------------------------------*/

#define TIMEOUT_MS                  10

#define LPS_LSB_PER_hPA             4096
#define LPS_LSB_PER_C               100

#define REG_INTERRUPT_CFG           0x0B
#define REG_THS_P_L                 0x0C
#define REG_THS_P_H                 0x0D
#define REG_IF_CTRL                 0x0E
#define REG_WHO_AM_I                0x0F
#define REG_CTRL_REG1               0x10
#define REG_CTRL_REG2               0x11
#define REG_CTRL_REG3               0x12
#define REG_FIFO_CTRL               0x13
#define REG_FIFO_WTM                0x14
#define REG_REF_P_L                 0x15
#define REG_REF_P_H                 0x16
#define REG_RPDS_L                  0x18
#define REG_RPDS_H                  0x19
#define REG_INT_SOURCE              0x24
#define REG_FIFO_STATUS1            0x25
#define REG_FIFO_STATUS2            0x26
#define REG_STATUS                  0x27
#define REG_PRESS_OUT_XL            0x28
#define REG_PRESS_OUT_L             0x29
#define REG_PRESS_OUT_H             0x2A
#define REG_TEMP_OUT_L              0x2B
#define REG_TEMP_OUT_H              0x2C
#define REG_FIFO_DATA_OUT_PRESS_XL  0x78
#define REG_FIFO_DATA_OUT_PRESS_L   0x79
#define REG_FIFO_DATA_OUT_PRESS_H   0x7A
#define REG_FIFO_DATA_OUT_TEMP_L    0x7B
#define REG_FIFO_DATA_OUT_TEMP_H    0x7C

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern SPI_HandleTypeDef LPS22HH_SPI_BUS;

static osMutexId lps_spi_mutex = NULL;
static osEventFlagsId_t lps_events = NULL;

static uint8_t rx_data;

static PressureBufferInfo_t p_buf_info = {NULL, 0, NULL};
static TemperatureBufferInfo_t t_buf_info = {NULL, 0, NULL};

static uint8_t last_ctrl_reg1 = 0;
static bool powered_down = true;

/* Private function prototypes -----------------------------------------------*/

static bool readPressure(void);
static bool readTemperature(void);

static bool regWrite(uint8_t address, uint8_t data);
static bool regRead(uint8_t address);
static bool regReadMulti(uint8_t address, uint8_t* out, uint8_t len);

static bool waitForMutex(void);
static void setSpiClearFlag(void);
static bool waitForSpiClearFlag(void);

/* Exported function definitions ---------------------------------------------*/

void LPS_CreateResources(void)
{
  if (lps_spi_mutex != NULL) REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);
  if (lps_events != NULL) REGISTER_ERROR(ERROR_FLAGS_INITIALIZATION);

  lps_spi_mutex = osMutexNew(NULL);
  if (lps_spi_mutex == NULL) REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);

  lps_events = osEventFlagsNew(NULL);
  if (lps_events == NULL) REGISTER_ERROR(ERROR_FLAGS_INITIALIZATION);
}

void LPS_Init(LpsOdr_t odr)
{
  RETURN_IF_ERROR_PRESENT();
  if (regWrite(REG_INTERRUPT_CFG, 0x00) == false) 
    REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  uint8_t data = 0;
  data |= odr << 4;
  data |= 1 << 3; // EN_LPFP: Enables LPF
  data |= 1 << 2; // LPFP_CFG: Decreases measurement bandwidth to ODR/20
  // data |= 1 << 1; // BDU: Ensures samples read correspond to the same physical sample
  // data |= 1 << 0; // SIM: To use SPI 3 wire
  last_ctrl_reg1 = data;
  if (regWrite(REG_CTRL_REG1, data) == false) 
    REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  data = 0;
  data |= 0 << 6; // Active high interrupt
  data |= 0 << 5; // Push-pull interrupt
  data |= 1 << 4; // IF_ADD_INC: Auto increment addres during multi-byte read
  data |= 1 << 1; // LOW_NOISE_EN: Prioritize less noise over low current since current consumption difference is relatively negligible
  data |= 0 << 0; // ONE_SHOT: Disable one shot mode
  if (regWrite(REG_CTRL_REG2, data) == false) 
    REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  data = 0;
  // Do not care about FIFO registers since BYPASS mode used
  data |= 1 << 2; // DRDY: Enables the DRDY signal to be used as an interrupt
  data |= 0b00 << 0; // INT_S: Use DRDY as a signal on INT_DRDY
  if (regWrite(REG_CTRL_REG3, data) == false) 
    REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  powered_down = false;
}

void LPS_RegisterPressureBuf(uint32_t* p_buf, uint16_t buf_len, uint16_t* buf_head)
{
  if ((p_buf == NULL) || (buf_len == 0) || (buf_head == 0)) 
    REGISTER_ERROR(ERROR_INVALID_FUNCTION_PARAMETERS);
  p_buf_info.buf = (RawPressureData_t*) p_buf;
  p_buf_info.len = buf_len;
  p_buf_info.head = buf_head;
  *buf_head = 0;
}

void LPS_RegisterTemperatureBuf(uint16_t* t_buf, uint16_t buf_len, uint16_t* buf_head)
{
  if ((t_buf == NULL) || (buf_len == 0) || (buf_head == 0)) 
    REGISTER_ERROR(ERROR_INVALID_FUNCTION_PARAMETERS);
  t_buf_info.buf = (RawTemperatureData_t*) t_buf;
  t_buf_info.len = buf_len;
  t_buf_info.head = buf_head;
  *buf_head = 0;
}

void LPS_PowerDown()
{
  if (regWrite(REG_CTRL_REG1, 0U) == false) REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  powered_down = true;
}

void LPS_PowerUp()
{
  if (regWrite(REG_CTRL_REG1, last_ctrl_reg1) == false) REGISTER_ERROR(ERROR_PRESSURE_SENSOR);

  powered_down = false;
}

bool LPS_CheckInterface()
{
  if (regRead(REG_WHO_AM_I) == false) REGISTER_ERROR_NON_VOID(ERROR_PRESSURE_SENSOR, false);

  return rx_data == 0xD3;
}

void LPS_ReadData()
{
  if ((p_buf_info.buf != NULL) && (p_buf_info.len != 0) && (p_buf_info.head != NULL)) {
    if (readPressure() == false) REGISTER_ERROR(ERROR_PRESSURE_SENSOR);
  }

  if ((t_buf_info.buf != NULL) && (t_buf_info.len != 0) && (t_buf_info.head != NULL)) {
    if (readTemperature() == false) REGISTER_ERROR(ERROR_PRESSURE_SENSOR);
  }
}

float LPS_ConvertRawPressure(uint32_t raw_reading)
{
  return ((float) raw_reading) / ((float) LPS_LSB_PER_hPA);
}

float LPS_ConvertRawTemperature(uint16_t raw_reading)
{
  return ((float) raw_reading) / ((float) LPS_LSB_PER_C);
}

// SPI callbacks

void LPS_TxComplete()
{
  setSpiClearFlag();
}

void LPS_RxComplete()
{
  setSpiClearFlag();
}

void LPS_IntDrdy()
{
  if (lps_events == NULL) {REGISTER_ERROR(ERROR_GENERAL_WARN_ISR); return;}
  osEventFlagsSet(lps_events, LPS_DATA_READY);
}

/* Private function definitions ----------------------------------------------*/

bool readPressure(void)
{
  uint8_t data[3];
  if (regReadMulti(REG_PRESS_OUT_XL, data, 3) == false) return false;

  RawPressureData_t pressure_data;
  pressure_data.raw_pressure = 0;
  pressure_data.registers.xl = data[0];
  pressure_data.registers.l  = data[1];
  pressure_data.registers.h  = data[2];

  p_buf_info.buf[*p_buf_info.head] = pressure_data;
  *p_buf_info.head = (*p_buf_info.head + 1) % p_buf_info.len;
  return true;
}

bool readTemperature(void)
{
  uint8_t data[2];
  if (regReadMulti(REG_TEMP_OUT_L, data, 2) == false) return false;

  RawTemperatureData_t temperature_data;
  temperature_data.registers.l = data[0];
  temperature_data.registers.h = data[1];

  t_buf_info.buf[*t_buf_info.head] = temperature_data;
  *t_buf_info.head = (*t_buf_info.head + 1) % t_buf_info.len;

  return true;
}

static uint8_t full_command[2];
bool regWrite(uint8_t address, uint8_t data)
{
  bool ret = true;
  if (waitForMutex() == false) return false;

  address &= 0x7F; // Set the MSB to 0 for W
  full_command[0] = address;
  full_command[1] = data;
  if (HAL_SPI_Transmit_IT(&LPS22HH_SPI_BUS, full_command, 2) != HAL_OK) {
    ret = false;
  }

  if (waitForSpiClearFlag() == false) ret = false;
  if (osMutexRelease(lps_spi_mutex) != osOK) ret = false;
  return ret;
}

static uint8_t spi_tx_buf[4];
static uint8_t spi_rx_buf[4];
bool regRead(uint8_t address)
{
  bool ret = true;
  if (waitForMutex() == false) return false;

  spi_tx_buf[0] = address | 0x80; // Set MSB to 1 for R
  spi_tx_buf[1] = 0x00;
  if (HAL_SPI_TransmitReceive_IT(&LPS22HH_SPI_BUS, spi_rx_buf, spi_rx_buf, 2) != HAL_OK) {
    ret = false;
  }

  if (waitForSpiClearFlag() == false) ret = false;
  if (ret == true) rx_data = spi_rx_buf[1];
  if (osMutexRelease(lps_spi_mutex) != osOK) ret = false;
  return ret;
}

bool regReadMulti(uint8_t address, uint8_t* out, uint8_t len)
{
  bool ret = true;
  if (len > sizeof(spi_tx_buf) - 1) return false;
  if (waitForMutex() == false) return false;

  spi_tx_buf[0] = address | 0x80;
  for (uint8_t i = 1; i <= len; i++) spi_tx_buf[i] = 0x00;

  if (HAL_SPI_TransmitReceive_IT(&LPS22HH_SPI_BUS, spi_tx_buf, spi_rx_buf, len + 1) != HAL_OK) {
    ret = false;
  }

  if (waitForSpiClearFlag() == false) ret = false;
  if (ret) {
    for (uint8_t i = 0; i < len; i++) out[i] = spi_rx_buf[i + 1];
  }
  if (osMutexRelease(lps_spi_mutex) != osOK) ret = false;
  return ret;
}

bool waitForMutex()
{
  if (lps_spi_mutex == NULL) {
    return false;
  }
  if (osMutexAcquire(lps_spi_mutex, TIMEOUT_MS) != osOK) {
    return false;
  }
  return true;
}

void setSpiClearFlag(void)
{
  if (lps_events == NULL) {REGISTER_ERROR(ERROR_GENERAL_WARN_ISR); return;}
  osEventFlagsSet(lps_events, LPS_SPI_CLEAR);
}

bool waitForSpiClearFlag(void)
{
  if (lps_events == NULL) return false;
  if (osEventFlagsWait(lps_events, LPS_SPI_CLEAR, osFlagsWaitAny, TIMEOUT_MS) & 0x80000000) return false;

  if (osEventFlagsClear(lps_events, LPS_SPI_CLEAR) & 0x80000000) return false;

  return true;
}
