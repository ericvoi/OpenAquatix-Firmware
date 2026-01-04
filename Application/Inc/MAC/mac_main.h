/*
 * mac_main.h
 *
 *  Created on: Sep 8, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MAC_MAC_MAIN_H_
#define MAC_MAC_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MAC_STATE_SUCCESS,
  MAC_STATE_DEFERRED,
  MAC_STATE_DROPPED,
  MAC_STATE_ERROR,
  MAC_STATE_IDLE
} MacState_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void MAC_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_MAIN_H_ */
