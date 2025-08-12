/*
 * PGA113-driver.c
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "pga113-driver.h"
#include "cmsis_os.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define PGA_CMD_READ      0x6a00
#define PGA_CMD_WRITE     0x2a00
#define PGA_CMD_NOP       0x0000
#define PGA_CMD_SDN_DIS   0xe100U
#define PGA_CMD_SDN_EN    0xe1f1

#define PGA_CHANNEL       0x0001 // CH1

#define MAX_TIMEOUT_MS    5

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern SPI_HandleTypeDef  hspi4;
extern DMA_HandleTypeDef  hdma_spi4_tx;
extern DMA_HandleTypeDef  hdma_spi4_rx;

static bool pga_is_shutdown;
static volatile PgaGain_t current_gain = PGA_GAIN_1; // POR value

static uint16_t tx_buffer __attribute__((section(".dma_buf")));
static uint16_t rx_buffer __attribute__((section(".dma_buf")));


/* Private function prototypes -----------------------------------------------*/

static bool transmitData(uint16_t *pData, uint16_t size);
static bool sendCommandReceiveResponse(uint16_t *pTxData, uint16_t *pRxData, uint16_t size);

bool Pga113_Init()
{
  pga_is_shutdown = false;

  return true;
}

void Pga113_SetGain(PgaGain_t gain)
{
  tx_buffer = PGA_CMD_WRITE;
  tx_buffer |= PGA_CHANNEL;
  tx_buffer |= (gain << 4);

  current_gain = gain;

  transmitData(&tx_buffer, 1);
}

bool Pga113_Read()
{
  tx_buffer = PGA_CMD_READ;

  return sendCommandReceiveResponse(&tx_buffer, &rx_buffer, 2);
}

bool Pga113_Shutdown()
{
  tx_buffer = PGA_CMD_SDN_EN;

  if (transmitData(&tx_buffer, 1) == true) {
    pga_is_shutdown = true;
    return true;
  }
  return false;
}

bool Pga113_Enable()
{
  tx_buffer = PGA_CMD_SDN_DIS;

  if (transmitData(&tx_buffer, 1) == true) {
    pga_is_shutdown = false;
    return true;
  }
  return false;
}

bool transmitData(uint16_t *pData, uint16_t size)
{
  uint16_t timeout = 0;
  while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) {
    if (timeout >= MAX_TIMEOUT_MS) {
      return false;
    }
    osDelay(1);
    timeout++;
  }

  return HAL_SPI_Transmit_DMA(&hspi4, (uint8_t*) pData, size) == HAL_OK;
}

bool sendCommandReceiveResponse(uint16_t *pTxData, uint16_t *pRxData, uint16_t size)
{
  uint16_t timeout = 0;
  while (HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY) {
    if (timeout >= MAX_TIMEOUT_MS) {
      return false;
    }
    osDelay(1);
    timeout++;
  }

  return HAL_SPI_TransmitReceive_DMA(&hspi4, (uint8_t*) pTxData, (uint8_t*) pRxData, size) == HAL_OK;
}

PgaGain_t Pga113_GetGain()
{
  return current_gain;
}
