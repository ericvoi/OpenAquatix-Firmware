/*
 * mess_feedback.c
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_adc.h"
#include "mess_feedback.h"
#include "usb_comm.h"
#include "dac_waveform.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define PRINT_CHUNK_SIZE    50

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t feedback_buffer[PROCESSING_BUFFER_SIZE];

static volatile uint16_t feedback_buffer_start_index = 0;
static volatile uint16_t feedback_buffer_end_index = 0;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Feedback_Init()
{
  feedback_buffer_start_index = 0;
  feedback_buffer_end_index = 0;
  if (ADC_RegisterFeedbackBuffer(feedback_buffer) == false) return false;

  return true;
}

void Feedback_IncrementEndIndex()
{
  feedback_buffer_end_index = (feedback_buffer_end_index + ADC_BUFFER_SIZE / 2) % PROCESSING_BUFFER_SIZE;
}

void Feedback_DumpData()
{
  char print_buffer[PRINT_CHUNK_SIZE * 7 + 1]; // Accommodates max uint16 length + \r\n + 1
  uint16_t print_index = 0;
  CDC_Transmit_HS((uint8_t*) "\b\b\r\n\r\n", 6);
  const uint16_t data_len = DAC_SAMPLE_RATE / 1000 * FEEDBACK_TEST_DURATION_MS;
  if (xSemaphoreTake(usbSemaphoreHandle, portMAX_DELAY) == pdTRUE) {
    for (uint16_t i = 0; i < data_len; i += PRINT_CHUNK_SIZE) {
      print_index = 0;

      for (uint16_t j = 0; j < PRINT_CHUNK_SIZE && (i + j) < data_len; j++) {
        print_index += sprintf(&print_buffer[print_index], "%u\r\n", feedback_buffer[i + j]);
      }

      CDC_Transmit_HS((uint8_t*) print_buffer, print_index);
      osDelay(10);
    }
  }
  xSemaphoreGive(usbSemaphoreHandle);
  memset(feedback_buffer, 0, PROCESSING_BUFFER_SIZE * sizeof(uint16_t));
  feedback_buffer_end_index = 0;
}

/* Private function definitions ----------------------------------------------*/
