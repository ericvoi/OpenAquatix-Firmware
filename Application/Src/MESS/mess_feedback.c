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



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Feedback_Init()
{
  ADC_FeedbackClear();
  return true;
}

void Feedback_DumpData()
{
  char print_buffer[PRINT_CHUNK_SIZE * 7 + 1]; // Accommodates max uint16 length + \r\n + 1
  uint16_t print_index = 0;
  COMM_TransmitData("\b\b\r\n\r\n", 6, COMM_USB);
  const uint16_t data_len = DAC_SAMPLE_RATE / 1000 * FEEDBACK_TEST_DURATION_MS;
  for (uint16_t i = 0; i < data_len; i += PRINT_CHUNK_SIZE) {
    print_index = 0;

    for (uint16_t j = 0; j < PRINT_CHUNK_SIZE && (i + j) < data_len; j++) {
      print_index += sprintf(&print_buffer[print_index], "%u\r\n", ADC_FeedbackGetDataAbsolute(i + j));
    }

    COMM_TransmitData(print_buffer, print_index, COMM_USB);
  }
  ADC_FeedbackClear();
}

/* Private function definitions ----------------------------------------------*/
