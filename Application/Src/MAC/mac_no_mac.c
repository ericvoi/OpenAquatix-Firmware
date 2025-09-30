/*
 * mac_no_mac.c
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mac_no_mac.h"
#include "mess_main.h"
#include "mac_protocol.h"
#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

extern osEventFlagsId_t channel_report_flag;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/



/* Private function definitions ----------------------------------------------*/

static void NoMac_Init(void* protocol_data)
{
  NoMacData_t* data = (NoMacData_t*) protocol_data;
  data->active_reservation = false;
  osEventFlagsSet(channel_report_flag, REPORT_NONE);
}

static void NoMac_Deinit(void* protocol_data)
{
  (void)(protocol_data);
}

static MacState_t NoMac_HandleTxRequest(void* protocol_data)
{
  NoMacData_t* data = (NoMacData_t*) protocol_data;
  if (data->active_reservation == true) {
    uint32_t current_timestamp = osKernelGetTickCount();
    if (current_timestamp - data->channel_reserved_until > MAXIMUM_RESERVATION_TIME_MS) {
      data->active_reservation = false;
      return MAC_STATE_SUCCESS;
    }
    return MAC_STATE_DEFERRED;
  }
  return MAC_STATE_SUCCESS;
}

static void NoMac_ProcessChannelReport(void* protocol_data, const ChannelReport_t report)
{
  (void)(protocol_data);
  (void)(report);
}

static MacState_t NoMac_ProcessRxMessage(void* protocol_data, const Message_t* message)
{
  NoMacData_t* data = (NoMacData_t*) protocol_data;
  MacState_t ret = MAC_STATE_SUCCESS;

  if (message->preamble.reservation_time_10ms.valid == true) {
    if (message->preamble.reservation_time_10ms.value > MAXIMUM_RESERVATION_TIME_MS / 10) {
      ret = MAC_STATE_ERROR;
    }
    uint32_t reserved_until = message->timestamp + message->preamble.reservation_time_10ms.value * 10;
    uint32_t current_timestamp = osKernelGetTickCount();
    if (current_timestamp - reserved_until > MAXIMUM_RESERVATION_TIME_MS) {
      data->active_reservation = false;
    }
    else {
      data->active_reservation = true;
      data->channel_reserved_until = reserved_until;
    }
  }

  if (MESS_AddMessageToRxQ(message) == false) {
    ret = MAC_STATE_ERROR;
  }

  return ret;
}

static MacState_t NoMac_EmergencyTx(void* protocol_data, const Message_t* message)
{
  (void) (protocol_data);
  if (MESS_AddMessageToTxQ(message) == false) {
    return MAC_STATE_ERROR;
  }

  return MAC_STATE_SUCCESS;
}

static const MacProtocolInterface_t no_mac = {
  .protocol = MAC_PROTOCOL_NONE,
  .init = NoMac_Init,
  .deinit = NoMac_Deinit,
  .handleTxRequest = NoMac_HandleTxRequest,
  .processChannelReport = NoMac_ProcessChannelReport,
  .processRxMessage = NoMac_ProcessRxMessage,
  .handleEmergencyTx = NoMac_EmergencyTx
};

const MacProtocolInterface_t* no_mac_interface = &no_mac;
