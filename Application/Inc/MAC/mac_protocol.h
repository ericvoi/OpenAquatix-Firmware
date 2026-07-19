/*
 * mac_protocol.h
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MAC_MAC_PROTOCOL_H_
#define MAC_MAC_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "mess_main.h"
#include "mac_main.h"
#include "mac_channel_reports.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define MAC_PROTOCOL_TABLE(X) \
  X(MAC_PROTOCOL_NONE, "No MAC") \
  X(MAC_PROTOCOL_CSMA_CA_BEB, "GA CSMA/CA with BEB (JANUS)") \

DECLARE_ENUM(MAC_PROTOCOL_TABLE, NUM_MAC_PROTOCOL, MacProtocol_t)

typedef struct {
  MacProtocol_t protocol;

  void (*init)(void* protocol_data);
  void (*deinit)(void* protocol_data);

  MacState_t (*handleTxRequest)(void* protocol_data);
  void (*processChannelReport)(void* protocol_data, const ChannelReport_t report);
  MacState_t (*processRxMessage)(void* protocol_data, const Message_t* message);

  MacState_t (*handleEmergencyTx)(void* protocol_data, const Message_t* message);
} MacProtocolInterface_t;

typedef struct {
  MacProtocol_t type;
  const MacProtocolInterface_t* interface;
  void* protocol_data;
  bool is_active;
} MacProtocolInstance_t;

/* Exported constants --------------------------------------------------------*/

#define MAC_PROTOCOL_UNKNOWN                        (NUM_MAC_PROTOCOL + 1)

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_PROTOCOL_H_ */
