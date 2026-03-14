/*
 * mess_ranging.c
 *
 *  Created on: Mar 11, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "mess_ranging.h"
#include "mess_main.h"
#include "mess_sync.h"
#include "mess_error_correction.h"
#include "mess_error_detection.h"
#include "main.h"
#include "core_cm7.h"
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define RANGING_TIMEOUT_MS            20000
#define RANGING_RESPONSE_DELAY        2000
#define AFE_TURNAROUND_TIME           500

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static bool outbound_ranging_request = false;
static uint32_t ranging_request_cyccnt;
static uint64_t ranging_request_timestamp;

extern osMessageQueueId_t mac_rx_queue;

/* Private function prototypes -----------------------------------------------*/

static void checkTimeout(void);
static bool isValidRequest(const DspConfig_t* cfg);

/* Exported function definitions ---------------------------------------------*/

void Ranging_Request(const DspConfig_t* cfg, bool feedback)
{
  if (isValidRequest(cfg) == false) {
    osEventFlagsSet(print_event_handle, MESS_FAILED_RANGING_REQUEST);
    return;
  }

  Message_t request_msg = {
    .protocol = PROTOCOL_CUSTOM,
    .data_type = RANGING_REQUEST,
    .type = feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER,
    .delay = false
  };

  request_msg.preamble.message_type.value = RANGING_REQUEST;
  request_msg.preamble.message_type.valid = true;

  if (MESS_AddMessageToTxQ(&request_msg) == false) {
    osEventFlagsSet(print_event_handle, MESS_FAILED_RANGING_REQUEST);
  }
}

void Ranging_LogRequest()
{
  uint32_t cyccnt = DWT->CYCCNT;
  checkTimeout();
  if (outbound_ranging_request == true) {
    // Already have an outgoing request. Overwrite for now
    // Notify later
  }
  outbound_ranging_request = true;
  ranging_request_cyccnt = cyccnt;
  ranging_request_timestamp = HAL_AbsoluteTimestamp();
}

void Ranging_Respond(uint32_t request_cyccnt, bool feedback)
{
  uint32_t additional_delay_cyccnt = RANGING_RESPONSE_DELAY * (SystemCoreClock / 1000);

  Message_t response_msg = {
    .protocol = PROTOCOL_CUSTOM,
    .data_type = RANGING_RESPONSE,
    .type = feedback ? MSG_TRANSMIT_FEEDBACK : MSG_TRANSMIT_TRANSDUCER,
    .delay = true,
    .delay_cyccnt = request_cyccnt + additional_delay_cyccnt
  };

  response_msg.preamble.message_type.value = RANGING_RESPONSE;
  response_msg.preamble.message_type.valid = true;

  if (MESS_AddMessageToTxQ(&response_msg) == false) {
    osEventFlagsSet(print_event_handle, MESS_FAILED_RANGING_RESPONSE);
  }
}

void Ranging_LogResponse(const Message_t* msg)
{
  checkTimeout();
  if (outbound_ranging_request == false) return; // Drop silently if not requested

  uint64_t current_timestamp = HAL_AbsoluteTimestamp();
  uint64_t timestamp_difference = current_timestamp - ranging_request_timestamp;
  uint64_t ms_for_wraparound = (ULONG_MAX) / (SystemCoreClock / 1000);

  // TODO: fix bug where uncertainty in timing could cause inaccuracies if
  // response took around a multiple of ULONG_MAX CYCCNT
  uint16_t num_wraparound = timestamp_difference / ms_for_wraparound;
  uint32_t cyccnt_difference = msg->rx_cyccnt - ranging_request_cyccnt;
  uint64_t cyccnt_total = cyccnt_difference + num_wraparound * ULONG_MAX;
  uint32_t fixed_delay_cyccnt = RANGING_RESPONSE_DELAY * (SystemCoreClock / 1000);
  int64_t one_way_cyccnt = ((int64_t)cyccnt_total - (int64_t)fixed_delay_cyccnt) / 2;
  float one_way_time = ((float) one_way_cyccnt) / ((float) SystemCoreClock);
  float one_way_range_m = one_way_time * SPEED_OF_SOUND_MPS;

  Message_t ranging_msg;
  memcpy(&ranging_msg, msg, sizeof(Message_t));
  ranging_msg.protocol = PROTOCOL_CUSTOM;
  ranging_msg.data_type = RANGING_RESPONSE;
  ranging_msg.range_m = one_way_range_m;

  if (osMessageQueuePut(mac_rx_queue, &ranging_msg, 0, 0) != osOK) {
    osEventFlagsSet(print_event_handle, MESS_RECEIVED_RANGING_RESPONSE_BAD);
  }
}

/* Private function definitions ----------------------------------------------*/

static void checkTimeout(void)
{
  if (outbound_ranging_request == false) return;

  uint64_t current_timestamp = HAL_AbsoluteTimestamp();
  uint64_t time_difference = current_timestamp - ranging_request_timestamp;
  if (time_difference >= RANGING_TIMEOUT_MS) {
    // TODO: notify user
    outbound_ranging_request = false;
  }
}

static bool isValidRequest(const DspConfig_t* cfg)
{
  if (cfg->protocol != PROTOCOL_CUSTOM) return false;
  uint16_t uncoded_bits = PACKET_PREAMBLE_LENGTH_BITS;
  uint16_t error_bits;
  ErrorDetection_CheckLength(&error_bits, cfg->preamble_validation);
  uncoded_bits += error_bits;
  uint16_t coded_bits = ErrorCorrection_CodedLength(uncoded_bits, cfg->preamble_ecc_method);
  coded_bits += Sync_NumSteps(cfg);

  uint32_t bit_duration_us = (uint32_t) (1.0E6 / (cfg->baud_rate));
  uint32_t packet_duration_us = coded_bits * bit_duration_us;
  uint16_t packet_duration_ms = packet_duration_us / 1000;

  if ((packet_duration_ms + AFE_TURNAROUND_TIME) > RANGING_RESPONSE_DELAY)
    return false;
  else
    return true;
}
