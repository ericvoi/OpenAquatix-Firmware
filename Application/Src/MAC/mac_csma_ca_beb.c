/*
 * mac_csma_ca_beb.c
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "mac_channel_reports.h"
#include "mac_main.h"
#include "mac_protocol.h"
#include "mac_csma_ca_beb.h"
#include "mess_main.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define SIGNIFICANT_ENERGY_THRESHOLD      (1.41253f) // 3 dB

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/



/* Private function definitions ----------------------------------------------*/

static void CsmaCaBeb_Init(void* protocol_data)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  data->state = MAC_STATE_IDLE;
  data->collision_count = 0;
  data->background_psd = 0;
  memset(&data->channel_reports, 0, sizeof(data->channel_reports));
  data->num_channel_reports = 0;
  data->channel_report_index = 0;
  bool sensing_complete = false;
}

static void CsmaCaBeb_Deinit(void* protocol_data)
{

}

static MacResult_t CsmaCaBeb_HandleTxRequest(void* protocol_data, const Message_t* message)
{

}

static void CsmaCaBeb_ProcessChannelReport(void* protocol_data, const ChannelReport_t report)
{

}

static void CsmaCaBeb_ProcessRxMessage(void* protocol_data, const Message_t* message)
{

}

static MacResult_t CsmaCaBeb_EmergencyTx(void* protocol_data, const Message_t* message)
{

}

static const MacProtocolInterface_t csma_ca_beb = {
  .protocol = MAC_PROTOCOL_CSMA_CA_BEB,
  .init = CsmaCaBeb_Init,
  .deinit = CsmaCaBeb_Deinit,
  .handleTxRequest = CsmaCaBeb_HandleTxRequest,
  .processChannelReport = CsmaCaBeb_ProcessChannelReport,
  .processRxMessage = CsmaCaBeb_ProcessRxMessage,
  .handleEmergencyTx = CsmaCaBeb_EmergencyTx
};

const MacProtocolInterface_t* csma_ca_beb_interface = &csma_ca_beb;
