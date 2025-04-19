/*
 * PGA113-driver.c
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "PGA113-driver.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define PGA_CMD_READ      0x6a00
#define PGA_CMD_WRITE     0x2a00
#define PGA_CMD_NOP       0x0000
#define PGA_CMD_SDN_DIS   0xe100U
#define PGA_CMD_SDN_EN    0xe1f1

#define PGA_CHANNEL       0x0001 // CH1

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern SPI_HandleTypeDef  hspi4;
extern DMA_HandleTypeDef  hdma_spi4_tx;
extern DMA_HandleTypeDef  hdma_spi4_rx;

volatile bool             spi4_dma_complete;
bool                      pga_is_shutdown;
volatile PgaGain_t        current_gain;

uint16_t tx_buffer;
uint16_t rx_buffer;


/* Private function prototypes -----------------------------------------------*/

HAL_StatusTypeDef SPI_TransmitData(uint16_t *pData, uint16_t size);
HAL_StatusTypeDef SPI_SendCommandReceiveResponse(uint16_t *pTxData, uint16_t *pRxData, uint16_t size);

HAL_StatusTypeDef PGA_Init()
{
  spi4_dma_complete = true;
  pga_is_shutdown = false;

  rx_buffer = 2;

  return HAL_OK;
}

void PGA_SetGain(PgaGain_t gain)
{
  tx_buffer = PGA_CMD_WRITE;
  tx_buffer |= PGA_CHANNEL;
  tx_buffer |= (gain << 4);

  current_gain = gain;

  SPI_TransmitData(&tx_buffer, 1);
}

HAL_StatusTypeDef PGA_Read()
{
  tx_buffer = PGA_CMD_READ;

  HAL_StatusTypeDef ret = SPI_SendCommandReceiveResponse(&tx_buffer, &rx_buffer, 2);

  return ret;
}

HAL_StatusTypeDef PGA_Update()
{
  return HAL_OK;
}

HAL_StatusTypeDef PGA_Shutdown()
{
  return HAL_OK;
}

HAL_StatusTypeDef PGA_Enable()
{
  tx_buffer = PGA_CMD_SDN_DIS;

  HAL_StatusTypeDef ret = SPI_TransmitData(&tx_buffer, 1);

  return ret;
}

HAL_StatusTypeDef PGA_Status()
{
  return HAL_OK;
}

HAL_StatusTypeDef SPI_TransmitData(uint16_t *pData, uint16_t size)
{
  // Wait for any ongoing transfer to complete
  while(HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

  // Transmit data using DMA
  return HAL_SPI_Transmit_DMA(&hspi4, (uint8_t*) pData, size);
}

HAL_StatusTypeDef SPI_SendCommandReceiveResponse(uint16_t *pTxData, uint16_t *pRxData, uint16_t size)
{
  // Wait for any ongoing transfer to complete
  while(HAL_SPI_GetState(&hspi4) != HAL_SPI_STATE_READY);

  // Transmit and receive data using DMA
  return HAL_SPI_TransmitReceive_DMA(&hspi4, (uint8_t*) pTxData, (uint8_t*) pRxData, size);
}

uint8_t PGA_GetGain()
{
  return current_gain;
}
