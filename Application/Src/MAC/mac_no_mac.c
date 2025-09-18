/*
 * mac_no_mac.c
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_main.h"
#include "mac_protocol.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/



/* Private function definitions ----------------------------------------------*/

static void NoMac_Init(void* protocol_data)
{

}

static void NoMac_Deinit(void* protocol_data)
{

}

static MacState_t NoMac_HandleTxRequest(void* protocol_data)
{

}

static void NoMac_ProcessChannelReport(void* protoocol_data, const ChannelReport_t report)
{

}

static void NoMac_ProcessRxMessage(void* protocol_data, const Message_t message)
{

}

static MacState_t NoMac_EmergencyTx(void* protocol_data, const Message_t* message)
{

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

const MacProtocolInterface_t no_mac_interface = &no_mac;
