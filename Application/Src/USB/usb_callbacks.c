/*
 * usb_callbacks.c
 *
 *  Created on: Mar 31, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "tusb.h"
#include "usb_main.h"
#include "hil_main.h"
#include "error_manager.h"
#include "hmi_usb.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define HMI_CDC_BUF_SIZE          64

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern osThreadId_t hil_taskHandle;

/* Private function prototypes -----------------------------------------------*/

static void cdcHmiCb(void);

/* Exported function definitions ---------------------------------------------*/

void tud_cdc_rx_cb(uint8_t itf)
{
  switch (itf) {
    case CDC_ITF_HMI:
      cdcHmiCb();
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void tud_vendor_rx_cb(uint8_t itf, uint8_t const* buffer, uint16_t bufsize)
{
  (void) (buffer);
  (void) (bufsize);
  switch (itf) {
    case VENDOR_ITF_HIL_STREAM:
      osThreadFlagsSet(hil_taskHandle, HIL_EVT_STREAM_RX_RDY);
      break;
    case VENDOR_ITF_HIL_CONTROL:
      osThreadFlagsSet(hil_taskHandle, HIL_EVT_CONTROL_CMD);
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes)
{
  (void) (sent_bytes);
  switch (itf) {
    case VENDOR_ITF_HIL_STREAM:
      osThreadFlagsSet(hil_taskHandle, HIL_EVT_STREAM_TX_CPLT);
      break;
    case VENDOR_ITF_HIL_CONTROL:
      break;
    default:
    REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

/* Private function definitions ----------------------------------------------*/

static void cdcHmiCb(void)
{
  uint8_t buf[HMI_CDC_BUF_SIZE];
  while (tud_cdc_n_available(CDC_ITF_HMI)) {
    uint32_t count = tud_cdc_n_read(CDC_ITF_HMI, buf, HMI_CDC_BUF_SIZE);
    USB_ProcessRxData(buf, count);
  }
}

void OTG_HS_EP1_OUT_IRQHandler(void)
{
  tud_int_handler(BOARD_TUD_RHPORT);
}

void OTG_HS_EP1_IN_IRQHandler(void)
{
  tud_int_handler(BOARD_TUD_RHPORT);
}

void OTG_HS_IRQHandler(void)
{
  tud_int_handler(BOARD_TUD_RHPORT);
}
