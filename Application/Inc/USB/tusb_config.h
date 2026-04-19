/*
 * tusb_config.h
 *
 *  Created on: Mar 30, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef INC_TUSB_CONFIG_H_
#define INC_TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUSB_MCU              OPT_MCU_STM32H7
#define BOARD_TUD_RHPORT          1
#define BOARD_TUD_MAX_SPEED       OPT_MODE_HIGH_SPEED
#define CFG_TUSB_RHPORT1_MODE     (OPT_MODE_DEVICE | OPT_MODE_HIGH_SPEED)

// ---- System ----
#define CFG_TUSB_OS               OPT_OS_FREERTOS
#define CFG_TUSB_MEM_ALIGN        __attribute__((aligned(4)))
#define CFG_TUSB_MEM_SECTION      __attribute__((section(".dma_buf")))

// ---- Device ----
#define CFG_TUD_ENABLED           1
#define CFG_TUD_ENDPOINT0_SIZE    64

// ---- Class ----
#define CFG_TUD_CDC               1
#define CFG_TUD_CDC_RX_BUFSIZE    1024
#define CFG_TUD_CDC_TX_BUFSIZE    1024
#define CFG_TUD_VENDOR            2
#define CFG_TUD_VENDOR_RX_BUFSIZE 2048
#define CFG_TUD_VENDOR_TX_BUFSIZE 2048

#define CFG_TUD_VENDOR_EPSIZE     512 // For HS

#define CFG_TUD_HID               0
#define CFG_TUD_MSC               0
#define CFG_TUD_MIDI              0

#define CFG_TUSB_DEBUG            0
#define CFG_TUD_DWC2_DMA_ENABLE   1

typedef enum {
  ITF_NUM_CDC = 0,
  ITF_NUM_CDC_DATA,
  ITF_NUM_HIL_STREAM,
  ITF_NUM_HIL_CONTROL,
  ITF_NUM_TOTAL
} TusbInterfaces_t;

#ifdef __cplusplus
}
#endif

#endif /* INC_TUSB_CONFIG_H_ */
