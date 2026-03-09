/*
 * mac_main.c
 *
 *  Created on: Sep 8, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "cfg_main.h"
#include "cfg_parameters.h"
#include "cfg_defaults.h"
#include "sys_error.h"
#include "mess_main.h"
#include "mac_csma_ca_beb.h"
#include "mac_no_mac.h"
#include "mac_protocol.h"
#include "cmsis_os.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  MacState_t state;

  MacProtocol_t current_protocol;
  MacProtocol_t requested_protocol;

  const MacProtocolInterface_t* interface;

  union {
    NoMacData_t no_mac_data;
    CsmaCaBebData_t csma_ca_beb_data;
  } protocol_data;
} MacTaskContext_t;

/* Private define ------------------------------------------------------------*/

#define REGULAR_TX_QUEUE_SIZE       5
#define EMERGENCY_TX_QUEUE_SIZE     3
#define RX_QUEUE_SIZE               1

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

DEFINE_DESC_TABLE(MAC_PROTOCOL_TABLE, mac_protocol_descriptions)

osMessageQueueId_t regular_tx_queue = NULL;
osMessageQueueId_t emergency_tx_queue = NULL;
osMessageQueueId_t mac_rx_queue = NULL;

static MacTaskContext_t task_context = {
  .state = MAC_STATE_IDLE,
  .current_protocol = MAC_PROTOCOL_UNKNOWN,
  .requested_protocol = DEFAULT_MAC,
  .interface = NULL
};

extern const MacProtocolInterface_t* csma_ca_beb_interface;
extern const MacProtocolInterface_t* no_mac_interface;

static const MacProtocolInterface_t* protocol_registry[NUM_MAC_PROTOCOL];

extern osMessageQueueId_t channel_report_queue;

/* Private function prototypes -----------------------------------------------*/

static void registerProtocols();

static bool registerMacParams();
static bool createTxQueues();
static bool createRxQueues();

static bool switchMacProtocol();

/* Exported function definitions ---------------------------------------------*/

void MAC_StartTask(void* argument)
{
  (void) (argument);

  if (Param_RegisterTask(MAC_TASK, "MAC") == false) {
    Error_Routine(ERROR_MAC_INIT);
  }

  if (registerMacParams() == false) {
    Error_Routine(ERROR_MAC_INIT);
  }

  if (Param_TaskRegistrationComplete(MAC_TASK) == false) {
    Error_Routine(ERROR_MAC_INIT);
  }

  if (createRxQueues() == false || createTxQueues() == false) {
    Error_Routine(ERROR_MAC_INIT);
  }

  registerProtocols();

  CFG_WaitLoadComplete();

  for (;;) {
    if (switchMacProtocol() == false) {
      Error_Routine(ERROR_MAC_PROCESSING);
    }

    ChannelReport_t channel_report;
    if (osMessageQueueGet(channel_report_queue, &channel_report, NULL, 0) == osOK) {
      if (task_context.interface->processChannelReport != NULL) {
        task_context.interface->processChannelReport(&task_context.protocol_data.csma_ca_beb_data, channel_report);
      }
    }

    if (osMessageQueueGetCount(regular_tx_queue) != 0) {
      if (task_context.interface->handleTxRequest != NULL) {
        task_context.state = task_context.interface->handleTxRequest(&task_context.protocol_data.csma_ca_beb_data);
        switch (task_context.state) {
          case MAC_STATE_DROPPED:
            // TODO: inform user
            break;
          case MAC_STATE_ERROR:
            Error_Routine(ERROR_MAC_PROCESSING);
            break;
          case MAC_STATE_SUCCESS: {
            // add to mess queue
            Message_t message;
            if (osMessageQueueGet(regular_tx_queue, &message, NULL, 0) != osOK) {
              Error_Routine(ERROR_MAC_PROCESSING);
              break;
            }
            if (MESS_AddMessageToTxQ(&message) == false) {
              Error_Routine(ERROR_MAC_PROCESSING);
            }
            break;
          }
          default:
            break;
        }
      }
    }

    if (osMessageQueueGetCount(emergency_tx_queue) != 0) {
      if (task_context.interface->handleEmergencyTx != NULL) {
        Message_t message_to_send;
        osMessageQueueGet(emergency_tx_queue, &message_to_send, NULL, 0);
        task_context.state = task_context.interface->handleEmergencyTx(&task_context.protocol_data.csma_ca_beb_data, &message_to_send);
      }
    }

    if (osMessageQueueGetCount(mac_rx_queue) != 0) {
      Message_t received_message;
      osMessageQueueGet(mac_rx_queue, &received_message, NULL, 0);
      if (task_context.interface->processRxMessage != NULL) {
        task_context.state = task_context.interface->processRxMessage(&task_context.protocol_data.csma_ca_beb_data, &received_message);
      }
    }

    osDelay(1);
  }
}

/* Private function definitions ----------------------------------------------*/

void registerProtocols()
{
  protocol_registry[MAC_PROTOCOL_NONE] = no_mac_interface;
  protocol_registry[MAC_PROTOCOL_CSMA_CA_BEB] = csma_ca_beb_interface;
}

bool registerMacParams()
{
  uint32_t min_u32 = MIN_MAC;
  uint32_t max_u32 = MAX_MAC;
  if (Param_Register(PARAM_MAC, "MAC method", PARAM_TYPE_ENUM, 
                     &task_context.requested_protocol, sizeof(MacProtocol_t),
                     &min_u32, &max_u32, NULL, mac_protocol_descriptions) == false) {
    return false;
  }
  return true;
}

bool createTxQueues()
{
  if (regular_tx_queue != NULL || emergency_tx_queue != NULL) {
    return false;
  }
  regular_tx_queue = osMessageQueueNew(REGULAR_TX_QUEUE_SIZE, sizeof(Message_t), NULL);
  emergency_tx_queue = osMessageQueueNew(EMERGENCY_TX_QUEUE_SIZE, sizeof(Message_t), NULL);

  if (regular_tx_queue == NULL || emergency_tx_queue == NULL) {
    return false;
  }
  return true;
}

bool createRxQueues()
{
  if (mac_rx_queue != NULL) {
    return false;
  }
  mac_rx_queue = osMessageQueueNew(RX_QUEUE_SIZE, sizeof(Message_t), NULL);

  return mac_rx_queue != NULL;
}

bool switchMacProtocol()
{
  if (task_context.current_protocol == task_context.requested_protocol) {
    return true;
  }

  if (task_context.current_protocol != MAC_PROTOCOL_UNKNOWN) {
    if (task_context.interface == NULL) {
      return false;
    }
    if (task_context.interface->deinit == NULL) {
      return false;
    }
    task_context.interface->deinit(&task_context.protocol_data);
  }

  for (uint16_t i = 0; i < sizeof(protocol_registry) / sizeof(protocol_registry[0]); i++) {
    if (protocol_registry[i]->protocol != task_context.requested_protocol) continue;
    
    protocol_registry[i]->init(&task_context.protocol_data);
    task_context.state = MAC_STATE_IDLE;
    task_context.interface = protocol_registry[i];
    task_context.current_protocol = protocol_registry[i]->protocol;
    return true;
  }

  return false;
}
