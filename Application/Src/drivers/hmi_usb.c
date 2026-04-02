/*
 * hmi_usb.c
 *
 *  Created on: Feb 1, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "hmi_usb.h"
#include "cmsis_os.h"
#include "comm_main.h"
#include "tusb.h"
#include "error_manager.h"
#include <string.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define USB_MUTEX_TIMEOUT             100

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static CommBuffer_t usb_buffer __attribute__((section(".dma_buf")));
static osMutexId_t usb_mutex;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void USB_CreateShared(void)
{
  if (usb_mutex != NULL) REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);

  usb_mutex = osMutexNew(NULL);
  if (usb_mutex == NULL) REGISTER_ERROR(ERROR_MUTEX_INITIALIZATION);;
}

void USB_Init(void)
{
  usb_buffer.length = MAX_COMM_IN_BUFFER_SIZE;
  usb_buffer.index = 0;
  usb_buffer.contents_changed = false;
  usb_buffer.data_ready = false;
  usb_buffer.source = COMM_USB;
}

void USB_TransmitData(uint8_t* data, uint16_t len)
{
  if (osMutexAcquire(usb_mutex, USB_MUTEX_TIMEOUT) != osOK)
    REGISTER_ERROR(ERROR_USB_HMI);

  uint32_t written = tud_cdc_n_write(ITF_NUM_CDC, data, len);
  tud_cdc_n_write_flush(ITF_NUM_CDC);
  
  osMutexRelease(usb_mutex);

  if (written != len) REGISTER_ERROR(ERROR_USB_HMI);
}

void USB_ProcessRxData(uint8_t* data, uint32_t len)
{
  if (usb_buffer.data_ready || len == 0) return;

  for (uint16_t i = 0; i < len; i++) {
    uint8_t c = data[i];
    bool at_capacity = (usb_buffer.index >= sizeof(usb_buffer.buffer) - 1);

    if (c == WITHDRAW_CHAR) {
      usb_buffer.buffer[0] = WITHDRAW_CHAR;
      usb_buffer.buffer[1] = '\0';
      usb_buffer.index = 1;
      usb_buffer.data_ready = true;
      usb_buffer.contents_changed = true;
    }
    else if (c == '\r' || c == '\n') {
      if (usb_buffer.index > 0) {
        usb_buffer.buffer[usb_buffer.index] = '\0';
        usb_buffer.data_ready = true;
        usb_buffer.contents_changed = true;
      }
    }
    else if (c == '\b') {
      if (usb_buffer.index > 0) {
        usb_buffer.index--;
        usb_buffer.contents_changed = true;
      }
    }
    // Buffer is not full, so add to buffer
    else if (at_capacity == false) {
      usb_buffer.buffer[usb_buffer.index++] = c;
      usb_buffer.contents_changed = true;
    }
    // else: at capacity, printable char -> silently discard

    if (usb_buffer.data_ready) return;
  }
}

RxState_t USB_GetHmiInput(uint8_t* buffer, uint16_t* len)
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
