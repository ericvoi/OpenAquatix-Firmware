/*
 * mac_protocol.h
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

#ifndef MAC_MAC_PROTOCOL_H_
#define MAC_MAC_PROTOCOL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "mess_main.h"
#include "mac_channel_reports.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MAC_PROTOCOL_NONE,
  MAC_PROTOCOL_CSMA_CA_BEB,
  NUM_MAC_PROTOCOL,

  MAC_PROTOCOL_UNKNOWN
} MacProtocol_t;

typedef enum {
  MAC_RESULT_SUCCESS,
  MAC_RESULT_DEFERRED,
  MAC_RESULT_DROPPED,
  MAC_RESULT_BUSY
} MacState_t;

typedef struct {
  MacProtocol_t protocol;

  void (*init)(void* protocol_data);
  void (*deinit)(void* protocol_data);

  MacState_t (*handleTxRequest)(void* protocol_data);
  void (*processChannelReport)(void* protocol_data, const ChannelReport_t report);
  void (*processRxMessage)(void* protocol_data, const Message_t* message);

  MacState_t (*handleEmergencyTx)(void* protocol_data, const Message_t* message);
} MacProtocolInterface_t;

typedef struct {
  MacProtocol_t type;
  const MacProtocolInterface_t* interface;
  void* protocol_data;
  bool is_active;
} MacProtocolInstance_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_PROTOCOL_H_ */
