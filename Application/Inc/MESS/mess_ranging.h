/*
 * mess_ranging.h
 *
 *  Created on: Mar 11, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_RANGING_H_
#define MESS_MESS_RANGING_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_dsp_config.h"
#include "mess_main.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Send a ranging request from the modem
 * 
 * @param feedback Whether to send on feedback network or transducer
 * @param cfg Configuration to use
 * 
 * @note This can fail if the configuration is not valid. Notification sent
 * through HMI
 */
void Ranging_Request(const DspConfig_t* cfg, bool feedback);

/**
 * @brief Logs the CYCCNT at which point a ranging request was sent
 */
void Ranging_LogRequest();

/**
 * @brief Responds to a ranging request by adding response to tx queue
 * 
 * @param request_cyccnt When the request was received
 * @param feedback Whether the request was received on feedback or not
 */
void Ranging_Respond(uint32_t request_cyccnt, bool feedback);

/**
 * @brief Logs reception of ranging response and notifies COMM task
 * 
 * @param msg The received message with the ranging response
 */
void Ranging_LogResponse(const Message_t* msg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_RANGING_H_ */
