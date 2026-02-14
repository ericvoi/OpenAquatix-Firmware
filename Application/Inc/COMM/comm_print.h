/*
 * comm_print.h
 *
 *  Created on: Feb 12, 2026
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMM_COMM_PRINT_H_
#define COMM_COMM_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "comm_main.h"
#include "mess_main.h"
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

typedef enum {
  CARGO_ERROR_DROP,
  CARGO_ERROR_PRINT,
  CARGO_ERROR_NOTIFY,
  NUM_CARGO_ERROR_BEHAVIORS
} CargoErrorBehavior_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Displays the message in a Message_t struct
 * 
 * @param msg The message to write to the HMI
 * @param out_buffer A temporary buffer used for writing outputs to HMI
 * @param interface Whether to send the message on USB, UART, or both
 */
void Print_DisplayReceivedMessage(Message_t* msg, uint8_t* out_buffer, CommInterface_t interface);

/**
 * @brief Registers parameters used for printing messages
 * 
 * @return true if all parameters successfully registered, false otherwise
 */
bool Print_RegisterParams(void);

#ifdef __cplusplus
}
#endif

#endif /* COMM_COMM_PRINT_H_ */
