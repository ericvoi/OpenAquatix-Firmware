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
#include "cmsis_os.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define SIGNIFICANT_ENERGY_THRESHOLD      (1.41253f) // 3 dB
#define OUTLIER_THRESHOLD                 (1.1f) // Threshold for channel report to be ignored

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)                         ((a < b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

extern osEventFlagsId_t channel_report_flag;
extern osMessageQueueId_t channel_report_queue;

/* Private function prototypes -----------------------------------------------*/

static float averageBackgroundNoise(CsmaCaBebData_t* data);

/* Exported function definitions ---------------------------------------------*/



/* Private function definitions ----------------------------------------------*/

static void CsmaCaBeb_Init(void* protocol_data)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  data->collision_count = 0;
  data->background_psd = 0;
  memset(&data->channel_reports, 0, sizeof(data->channel_reports));
  data->num_channel_reports = 0;
  data->channel_report_index = 0;
  bool sensing_complete = false;

  osEventFlagsSet(channel_report_flag, REPORT_16_CD_PSD);
}

static void CsmaCaBeb_Deinit(void* protocol_data)
{
  (void) (protocol_data);
  osEventFlagsSet(channel_report_flag, REPORT_NONE);
}

static MacState_t CsmaCaBeb_HandleTxRequest(void* protocol_data)
{

}

static void CsmaCaBeb_ProcessChannelReport(void* protocol_data, const ChannelReport_t report)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  // Reject outliers with too high of an energy
  if (data->sensing_complete == true) {
    if (report.psd > (data->background_psd * SIGNIFICANT_ENERGY_THRESHOLD)) {
      data->last_report_busy = true;
    }
    else {
      data->last_report_busy = false;
    }
    data->fresh_report = true;
    if (report.psd > (data->background_psd * OUTLIER_THRESHOLD)) {
      return;
    }
  }

  data->channel_reports[data->channel_report_index] = report.psd;
  data->channel_report_index = (data->channel_report_index + 1) % STORED_CSMA_CHANNEL_REPORTS;
  data->num_channel_reports = MIN(data->num_channel_reports + 1, CSMA_REPORTS_IN_BACKGROUND_NOISE);

  // Enough channel reports have been received for an estimation of the background noise
  if (data->num_channel_reports >= CSMA_REPORTS_IN_BACKGROUND_NOISE) {
    data->sensing_complete = true;
    data->background_psd = averageBackgroundNoise(data);
  }
}

static void CsmaCaBeb_ProcessRxMessage(void* protocol_data, const Message_t* message)
{

}

static MacState_t CsmaCaBeb_EmergencyTx(void* protocol_data, const Message_t* message)
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

float averageBackgroundNoise(CsmaCaBebData_t* data)
{
  float background_noise = 0.0f;
  for (uint16_t i = 0; i < data->num_channel_reports; i++) {
    uint16_t index = (data->channel_report_index + STORED_CSMA_CHANNEL_REPORTS 
                    - data->num_channel_reports - 1) % STORED_CSMA_CHANNEL_REPORTS;
    background_noise += data->channel_reports[index];
  }
  return background_noise /= data->num_channel_reports;
}
