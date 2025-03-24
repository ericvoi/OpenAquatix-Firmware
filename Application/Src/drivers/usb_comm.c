/*
 * usb_comm.c
 *
 *  Created on: Feb 1, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "usb_comm.h"
#include "usbd_cdc_if.h"
#include "cmsis_os.h"
#include "comm_main.h"
#include "cmsis_os.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define USB_RX_BUFFER_SIZE  128
#define USB_OVERFLOW_MESS   "Too many input characters!\r\n"

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint16_t usb_overflow_mess_len;
static CommBuffer_t usb_buffer;
//SemaphoreHandle_t usbMutex;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void USB_Init(void)
{
  usb_overflow_mess_len = strlen(USB_OVERFLOW_MESS);
  usb_buffer.length = MAX_COMM_IN_BUFFER_SIZE;
  usb_buffer.index = 0;
  usb_buffer.contents_changed = false;
  usb_buffer.data_ready = false;
  usb_buffer.source = COMM_USB;
}

void USB_TransmitData(uint8_t* data, uint16_t len)
{
  if (xSemaphoreTake(usbSemaphoreHandle, portMAX_DELAY) == pdTRUE) {
    CDC_Transmit_HS(data, len);
    vTaskDelay(pdMS_TO_TICKS(1));
    xSemaphoreGive(usbSemaphoreHandle);
  }
}

void USB_ProcessRxData(uint8_t* data, uint32_t len)
{
  // if a message is ready, do not process any more user input
  if (usb_buffer.data_ready == true) return;
  if (len == 0) return;
  usb_buffer.contents_changed = true;

  for (uint16_t i = 0; i < len; i++) {
    if (usb_buffer.index < sizeof(usb_buffer.buffer) - 1) {
      if (data[i] == '\e') {
        usb_buffer.buffer[0] = '\e';
        usb_buffer.buffer[1] = '\0';
        usb_buffer.index = 1;
        usb_buffer.data_ready = true;

        continue;
      }

      if (data[i] == '\b') {
        if (usb_buffer.index == 0) {
          continue;
        }
        else {
          usb_buffer.index--;
        }
//        USB_TransmitData((uint8_t*) "\b ", 2);
        continue;
      }
      if (data[i] == '\r' || data[i] == '\n') {

        if (usb_buffer.index > 0) {
          // End string
          usb_buffer.buffer[usb_buffer.index] = '\0';

          usb_buffer.data_ready = true;
        }
      }
      else {
        usb_buffer.buffer[usb_buffer.index++] = data[i];
      }
    } else {
      // Buffer overflow occurred - reset index and discard additional characters
      // An overflow message should be echoed to the user (handled elsewhere)
      usb_buffer.index = 0;
    }
  }
}

RxState_t USB_GetMessage(uint8_t* buffer, uint16_t* len)
{
  if (usb_buffer.contents_changed == false) return NO_CHANGE;
  RxState_t state = (usb_buffer.data_ready == true) ? DATA_READY : NEW_CONTENT;
  *len = usb_buffer.index;
  memcpy(buffer, usb_buffer.buffer, usb_buffer.index + 1); // +1 for null terminator
  usb_buffer.data_ready = false;
  if (state == DATA_READY) usb_buffer.index = 0;
  usb_buffer.contents_changed = false;
  return state;
}


/* Private function definitions ----------------------------------------------*/
