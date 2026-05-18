/*
 * hil_stream.c
 *
 *  Created on: Apr 5, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "hil_stream.h"
#include "hil_buffer.h"
#include "hil_manager.h"
#include "mess_filt_resources.h"
#include "dac_waveform.h"
#include "hmi_usb.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define ADC_DAC_DMA_BUF_SIZE              1024
#define HIL_TIMER                         htim6

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static volatile uint16_t dma_buf[ADC_DAC_DMA_BUF_SIZE] __attribute__((section(".dma_buf")));

extern ADC_HandleTypeDef FEEDBACK_ADC;
extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef HIL_TIMER;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void HilStream_StartAdc(void)
{
  HAL_TIM_Base_Stop(&HIL_TIMER);
  HAL_ADC_Stop_DMA(&FEEDBACK_ADC);
  osDelay(1);

  HAL_ADC_Start_DMA(&FEEDBACK_ADC, (uint32_t*) &dma_buf, ADC_DAC_DMA_BUF_SIZE);
}

void HilStream_StartDac(void)
{
  HAL_TIM_Base_Stop(&HIL_TIMER);
  HAL_StatusTypeDef st_stop = HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
  osDelay(1);

  HAL_StatusTypeDef st_start = HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t*) &dma_buf, ADC_DAC_DMA_BUF_SIZE, DAC_ALIGN_12B_R);
  HAL_TIM_Base_Start(&HIL_TIMER);

  char buf[64];
  int n = snprintf(buf, sizeof(buf),
                   "\r\n[DBG %lu] StartDac stop=%d start=%d\r\n",
                   (unsigned long) osKernelGetTickCount(),
                   (int) st_stop, (int) st_start);
  if (n > 0) USB_TransmitData((uint8_t*) buf, (uint16_t) n);
}

void HilStream_StopAdc(void)
{
  HAL_ADC_Stop_DMA(&FEEDBACK_ADC);
}

void HilStream_StopDac(void)
{
  HAL_StatusTypeDef st = HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);

  char buf[64];
  int n = snprintf(buf, sizeof(buf),
                   "\r\n[DBG %lu] StopDac wfRun=%d ret=%d\r\n",
                   (unsigned long) osKernelGetTickCount(),
                   (int) Waveform_IsRunning(), (int) st);
  if (n > 0) USB_TransmitData((uint8_t*) buf, (uint16_t) n);
}

// Fill samples in dma buffer from ring buffer
void HilStream_DacCallback(bool first_half)
{
  // Drop stale DAC events that may have been raised by a callback that fired
  // just before HilStream_StopDac() ran. The shared dma_buf is now being
  // written by the ADC (or is idle), so reading/writing it here would race.
  if (HilManager_HilMode() != HIL_STATE_RX) return;

  uint16_t start_index = (first_half) ? (0) : ADC_DAC_DMA_BUF_SIZE / 2;

  HilBuf_GetData(&dma_buf[start_index], ADC_DAC_DMA_BUF_SIZE / 2, 1 << 11);

  // Try to receive additional data
  HilBuf_ReadRxPackets();
}

// Add samples from ring buffer to dma
void HilStream_AdcCallback(bool first_half)
{
  if (HilManager_HilMode() != HIL_STATE_TX) return;

  uint16_t start_index = (first_half) ? (0) : ADC_DAC_DMA_BUF_SIZE / 2;

  HilBuf_AddData(&dma_buf[start_index], ADC_DAC_DMA_BUF_SIZE / 2);

  // Try to send a packet
  HilBuf_SendTxPackets();
}

/* Private function definitions ----------------------------------------------*/
