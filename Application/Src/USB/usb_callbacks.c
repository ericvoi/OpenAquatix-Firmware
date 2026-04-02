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
#include "hmi_usb.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define HMI_CDC_BUF_SIZE          64

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/

static void cdcHmiCb(void);

/* Exported function definitions ---------------------------------------------*/

void tud_cdc_rx_cb(uint8_t itf)
{
  if (itf == ITF_NUM_CDC) {
    cdcHmiCb();
  }
}

/* Private function definitions ----------------------------------------------*/

static void cdcHmiCb(void)
{
  uint8_t buf[HMI_CDC_BUF_SIZE];
  uint32_t count = tud_cdc_n_read(ITF_NUM_CDC, buf, HMI_CDC_BUF_SIZE);
  USB_ProcessRxData(buf, count);
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
