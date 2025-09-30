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
#include "cfg_main.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define SIGNIFICANT_ENERGY_THRESHOLD      (2.0f) // 3 dB
#define OUTLIER_THRESHOLD                 (1.1f) // Threshold for channel report to be ignored

#define CSMA_TIMEOUT_MS                   (10000) // Timeout for when the MAC task has not analyzed the background noise in sufficient time

/* Private macro -------------------------------------------------------------*/

#define MIN(a, b)                         ((a < b) ? (a) : (b))

/* Private variables ---------------------------------------------------------*/

extern osEventFlagsId_t channel_report_flag;
extern osMessageQueueId_t channel_report_queue;

static const float transmission_probability[7] = {1.0f / (1.0f + 0.5f), 1.0f / (1.0f + 1.0f), 1.0f / (1.0f + 2.0f), 0.2f, 0.2f, 0.2f, 0.2f}; // 2/3, 1/2, 1/3, 1/5, 1/5, 1/5
static const uint8_t num_probabilities = (sizeof(transmission_probability) / sizeof(transmission_probability[0]));

/* Private function prototypes -----------------------------------------------*/

static float averageBackgroundNoise(CsmaCaBebData_t* data);
static MacState_t timeout(CsmaCaBebData_t* data);
static void checkReservation(CsmaCaBebData_t* data);
static MacState_t probabilisticTransmission(CsmaCaBebData_t* data);

/* Exported function definitions ---------------------------------------------*/



/* Private function definitions ----------------------------------------------*/

static void CsmaCaBeb_Init(void* protocol_data)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  data->backoff_state = NO_BACKOFF;
  data->C = 0;
  data->reports_remaining_in_slot = CSMA_REPORTS_IN_SLOT;
  data->background_psd = 0;
  memset(&data->channel_reports, 0, sizeof(data->channel_reports));
  data->num_channel_reports = 0;
  data->channel_report_index = 0;
  data->sensing_complete = false;
  data->active_reservation = false;
  data->message_deferred = false;

  while (channel_report_flag == NULL) {
    osDelay(1);
  }
  osEventFlagsSet(channel_report_flag, REPORT_16_CD_PSD);
  CFG_IncrementVersionNumber();
}

static void CsmaCaBeb_Deinit(void* protocol_data)
{
  (void) (protocol_data);
  osEventFlagsSet(channel_report_flag, REPORT_NONE);
}

// Checks if there is an active reservation, whether the channel is busy and
// decides whether to add the message to the tx queue based on that
static MacState_t CsmaCaBeb_HandleTxRequest(void* protocol_data)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  // Background noise sensing not complete. Fail if its gone on too long
  if (data->sensing_complete == false) {
    return timeout(data);
  }

  checkReservation(data);
  if (data->active_reservation == true) {
    return MAC_STATE_DEFERRED;
  }

  switch (data->backoff_state) {
    case NO_BACKOFF:
      if (data->last_report_busy == true) {
        data->backoff_state = CONTINUOUS_BACKOFF;
        data->fresh_report = false;
        return MAC_STATE_DEFERRED;
      }
      else {
        return MAC_STATE_SUCCESS;
      }
    case CONTINUOUS_BACKOFF:
      if (data->fresh_report == false) {
        return MAC_STATE_DEFERRED;
      }
      data->fresh_report = false;
      data->C = 1;
      if (data->last_report_busy == true) {
        return MAC_STATE_DEFERRED;
      }
      else {
        data->backoff_state = SLOT_BACKOFF;
        data->busy_slot = false;
        data->reports_remaining_in_slot = CSMA_REPORTS_IN_SLOT;
        #ifdef JANUS_BAND_A
        return MAC_STATE_DEFERRED;
        #else
        return probabilisticTransmission(data);
        #endif
      }
    case SLOT_BACKOFF:
      if (data->fresh_report == false) {
        return MAC_STATE_DEFERRED;
      }
      data->fresh_report = false;
      data->busy_slot |= data->last_report_busy;
      data->reports_remaining_in_slot -= 1;
      // Received an entire slot so attempt transmission
      if (data->reports_remaining_in_slot == 0) {
        data->C = (data->busy_slot == true) ? (data->C + 1) : data->C;
        if (data->C >= 8) {
          return MAC_STATE_DROPPED;
        }
        data->reports_remaining_in_slot = CSMA_REPORTS_IN_SLOT;
        // Cannot immediately transmit after busy condition in band A
        #ifdef JANUS_BAND_A
        if (data->busy_slot == true) {
          return MAC_STATE_DEFERRED;
        } 
        #endif
        return probabilisticTransmission(data);
      }
      return MAC_STATE_DEFERRED;
    default:
      return MAC_STATE_ERROR;
  }
  return MAC_STATE_ERROR;
}

static void CsmaCaBeb_ProcessChannelReport(void* protocol_data, const ChannelReport_t report)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;

  if (data->sensing_complete == true) {
    // Reject outliers with too high of an energy
    if (report.psd > (data->background_psd * SIGNIFICANT_ENERGY_THRESHOLD)) {
      data->last_report_busy = true;
    }
    else {
      data->last_report_busy = false;
    }
    data->fresh_report = true;
    // Slow tracking of background noise
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

  checkReservation(data);
}

// Forward any received messages to COMM task taking note of any reservation times
static MacState_t CsmaCaBeb_ProcessRxMessage(void* protocol_data, const Message_t* message)
{
  CsmaCaBebData_t* data = (CsmaCaBebData_t*) protocol_data;
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

// Immediately send to MESS task
static MacState_t CsmaCaBeb_EmergencyTx(void* protocol_data, const Message_t* message)
{
  (void) (protocol_data);
  if (MESS_AddMessageToTxQ(message) == false) {
    return MAC_STATE_ERROR;
  }

  return MAC_STATE_SUCCESS;
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
    uint16_t index = (i + data->channel_report_index + STORED_CSMA_CHANNEL_REPORTS
                    - data->num_channel_reports) % STORED_CSMA_CHANNEL_REPORTS;
    background_noise += data->channel_reports[index];
  }
  return background_noise /= data->num_channel_reports;
}

MacState_t timeout(CsmaCaBebData_t* data)
{
  if (data->message_deferred == false) {
    data->message_deferred = true;
    data->deferral_timestamp = osKernelGetTickCount();
  }

  uint32_t current_timestamp = osKernelGetTickCount();
  if (current_timestamp - data->deferral_timestamp > CSMA_TIMEOUT_MS) {
    return MAC_STATE_ERROR;
  }
  else {
    return MAC_STATE_DEFERRED;
  }
}

void checkReservation(CsmaCaBebData_t* data)
{
  if (data->active_reservation == true) {
    uint32_t current_timestamp = osKernelGetTickCount();
    if (current_timestamp - data->channel_reserved_until > MAXIMUM_RESERVATION_TIME_MS) {
      data->active_reservation = false;
    }
  }
}

MacState_t probabilisticTransmission(CsmaCaBebData_t* data)
{
  if (data->C > num_probabilities) {
    return MAC_STATE_ERROR;
  }
  uint32_t current_timestamp = osKernelGetTickCount();
  srand(current_timestamp && 0xFFFF);
  float random_value = ((float) rand()) / ((float) RAND_MAX);

  if (random_value > transmission_probability[data->C - 1]) {
    return MAC_STATE_DEFERRED;
  }
  else {
    data->backoff_state = NO_BACKOFF;
    return MAC_STATE_SUCCESS;
  }
}
