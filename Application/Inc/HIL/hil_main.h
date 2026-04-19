/*
 * hil_main.h
 *
 *  Created on: Apr 2, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef HIL_HIL_MAIN_H_
#define HIL_HIL_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/

typedef enum {
  HIL_EVT_STREAM_RX_RDY = 1 << 0,
  HIL_EVT_STREAM_TX_CPLT = 1 << 1,
  HIL_EVT_ADC_HALF_FULL = 1 << 2,
  HIL_EVT_ADC_FULL = 1 << 3,
  HIL_EVT_DAC_HALF_FULL = 1 << 4,
  HIL_EVT_DAC_FULL = 1 << 5,
  HIL_EVT_CONTROL_CMD = 1 << 6,
  HIL_EVT_CAL_DONE = 1 << 7,
  HIL_EVT_ENTER_RX = 1 << 8,
  HIL_EVT_ENTER_TRANSITION = 1 << 9,
  HIL_EVT_ENTER_TX = 1 << 10,
} HilThreadFlags_t;

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define HIL_EVT_ALL               0xFFFFU

#define HIL_SAMPLING_RATE         500000

#define USB_HS_PACKET_SIZE        512

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Hardware-in-the-loop (HIL) task that fills buffers, updates HIL 
 * state, and processes commands
 * 
 * @param argument Task argument (unused)
 * 
 * @note This is a long-running FreeRTOS task
 */
void HIL_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_MAIN_H_ */
