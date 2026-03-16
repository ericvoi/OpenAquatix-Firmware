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

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Sets the AFE to known state (idle)
 */
void AFE_Init(void);

/**
 * @brief Set the AFE mode
 * 
 * @param new_mode New mode
 *        - AFE_MODE_IDLE Not receiving or sending (low-power)
 *        - AFE_MODE_RX Receiving a message from transducer
 *        - AFE_MODE_RX_FEEDBACK Receiving a message from feedback
 *        - AFE_MODE_TX Sending a message through transducer
 *        - AFE_MODE_TX_FEEDBACK Sending a message through transducer and listening to feedback
 */
void AFE_SetMode(AfeMode_t new_mode);

/**
 * @brief Returns current AFE mode
 * 
 * @return AfeMode_t current AFE mode
 */
AfeMode_t AFE_GetMode(void);

/**
 * @brief Check if AFE is set to transmit on transducer
 * 
 * @return true if transmitting on transducer, false otherwise
 */
bool AFE_IsTransmitting(void);

/**
 * @brief Check if AFE is set to receive feedback or transducer messages
 * 
 * @return true if a message can be received, false otherwise
 */
bool AFE_IsReceiving(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_AFE_H_ */
