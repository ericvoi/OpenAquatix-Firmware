/*
 * usb_main.h
 *
 *  Created on: Mar 31, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef USB_USB_MAIN_H_
#define USB_USB_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/

// Varies based on the order in which these are initialized in usb_descriptors.c
#define CDC_ITF_HMI             0

#define VENDOR_ITF_HIL_STREAM   0
#define VENDOR_ITF_HIL_CONTROL  1

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief TinyUSB task
 * 
 * @param argument Task argument (unused)
 * 
 * @note This is a long-running FreeRTOS task
 */
void USB_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* USB_USB_MAIN_H_ */
