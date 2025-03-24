/*
 * dau_card-driver.c
 *
 *  Created on: Mar 17, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "dau_card-driver.h"
#include "comm_main.h"
#include <stdbool.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define DAU_RX_BUFFER_SIZE  2048
#define DAU_TX_BUFFER_SIZE  2048

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static CommBuffer_t dau_buffer;
extern osMutexId_t dau_uart_mutexHandle;

static uint8_t rx_buffer[DAU_RX_BUFFER_SIZE];
static volatile uint32_t rx_head;
static volatile uint32_t rx_tail;

static uint8_t tx_buffer[DAU_TX_BUFFER_SIZE];
static volatile uint8_t tx_busy = 0;

extern UART_HandleTypeDef huart5;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void DAU_Init(void)
{
  dau_buffer.length = MAX_COMM_IN_BUFFER_SIZE;
  dau_buffer.index = 0;
  dau_buffer.data_ready = false;
  dau_buffer.contents_changed = false;
  dau_buffer.source = COMM_UART;

  __HAL_UART_ENABLE_IT(&huart5, UART_IT_IDLE);

  HAL_StatusTypeDef ret = HAL_UART_Receive_DMA(&huart5, rx_buffer, DAU_RX_BUFFER_SIZE);
}

void DAU_TransmitData(uint8_t* data, uint16_t len)
{
  if (osMutexAcquire(dau_uart_mutexHandle, osWaitForever) == osOK) {
    while (tx_busy == 1) {
      // TODO: Replace busy-wait with proper timeout or notification mechanism
      osDelay(1);
    }
    if (len > DAU_TX_BUFFER_SIZE) {
      osMutexRelease(dau_uart_mutexHandle);
      return;
    }

    memcpy(tx_buffer, data, len);

    tx_busy = 1;

    HAL_UART_Transmit_DMA(&huart5, tx_buffer, len);
    osMutexRelease(dau_uart_mutexHandle);
  }
}

void DAU_GetNewData()
{
  // Calculate current DMA position
  uint32_t dma_position = (DAU_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart5.hdmarx)) % DAU_RX_BUFFER_SIZE;
  rx_tail = dma_position; // Update tail to current DMA position

  // Process all data between head and tail
  while (rx_head != rx_tail) {
    uint8_t byte = rx_buffer[rx_head];

    DAU_ProcessRxData(&byte, 1);

    rx_head = (rx_head + 1) % DAU_RX_BUFFER_SIZE;
  }
}

void DAU_ProcessRxData(uint8_t* data, uint32_t len)
{
  // if a message is ready, do not process any more user input
  if (*data == '\0') return; // TODO: Review null character handling
  if (dau_buffer.data_ready == true) return;
  if (len == 0) return;
  dau_buffer.contents_changed = true;

  for (uint16_t i = 0; i < len; i++) {
    if (dau_buffer.index < sizeof(dau_buffer.buffer) - 1) {
      if (data[i] == '\e') {
        dau_buffer.buffer[0] = '\e';
        dau_buffer.buffer[1] = '\0';
        dau_buffer.index = 1;
        dau_buffer.data_ready = true;

        continue;
      }

      if (data[i] == '\b') {
        if (dau_buffer.index == 0) {
          continue;
        }
        else {
          dau_buffer.index--;
        }
        continue;
      }
      if (data[i] == '\r' || data[i] == '\n') {

        if (dau_buffer.index > 0) {
          // End string
          dau_buffer.buffer[dau_buffer.index] = '\0';

          dau_buffer.data_ready = true;
        }
      }
      else {
        dau_buffer.buffer[dau_buffer.index++] = data[i];
      }
    } else {
      dau_buffer.index = 0;
    }
  }
}

RxState_t DAU_GetMessage(uint8_t* buffer, uint16_t* len)
{
  if (dau_buffer.contents_changed == false) return NO_CHANGE;
  RxState_t state = (dau_buffer.data_ready == true) ? DATA_READY : NEW_CONTENT;
  *len = dau_buffer.index;
  memcpy(buffer, dau_buffer.buffer, dau_buffer.index + 1); // +1 for null terminator
  dau_buffer.data_ready = false;
  if (state == DATA_READY) dau_buffer.index = 0;
  dau_buffer.contents_changed = false;
  return state;
}

/* Private function definitions ----------------------------------------------*/

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == &huart5) {
    tx_busy = 0;
  }
}
