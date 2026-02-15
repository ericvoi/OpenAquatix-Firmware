/*
 * afe.h
 *
 *  Created on: Dec 31, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef AFE_AFE_H_
#define AFE_AFE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  AFE_MODE_IDLE,
  AFE_MODE_RX,
  AFE_MODE_RX_FEEDBACK,
  AFE_MODE_TX,
  AFE_MODE_TX_FEEDBACK
} AfeMode_t;

typedef enum {
  AFE_OK,
  AFE_INVALID_TRANSITION,
  AFE_TRANSITION_TIMEOUT,
  AFE_ERROR
} AfeStatus_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void AFE_Init(void);
AfeStatus_t AFE_SetMode(AfeMode_t new_mode);
AfeMode_t AFE_GetMode(void);
bool AFE_IsTransmitting(void);
bool AFE_IsReceiving(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_AFE_H_ */
