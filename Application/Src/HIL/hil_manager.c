/*
 * hil_manager.c
 *
 *  Created on: Apr 3, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "hil_manager.h"
#include "hil_main.h"
#include "hil_stream.h"
#include "hil_buffer.h"
#include "mess_hil_cal.h"
#include "mess_filt_resources.h"
#include "mess_main.h"
#include "dac_waveform.h"
#include "tusb.h"
#include "error_manager.h"
#include "usb_main.h"
#include "cmsis_os.h"
#include "stm32h7xx_hal.h"
#include <stdint.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define CMD_SIZE              64
#define RESPONSE_SIZE         64

_Static_assert((sizeof(HilCommandPacket_t) == CMD_SIZE), "Invalid HIL command packet size");
_Static_assert((sizeof(HilCalibrationPacket_t) == RESPONSE_SIZE), "Invalid HIL response packet size");
_Static_assert((sizeof(HilStatus_t) == RESPONSE_SIZE), "Invalid HIL status packet size");

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static HilState_t hil_state = HIL_STATE_IDLE;

/* Private function prototypes -----------------------------------------------*/

static void processHilCommand(const HilCommandPacket_t* cmd);

static void pingRespond(void);
static void enterHilMode(void);
static void exitHilMode(void);
static void resetHil(void);

static void enterHilRxMode(void);
static void enterHilTxMode(void);
static void enterHilTransitionMode(void);

static void sendStateTransitionPacket(void);

/* Exported function definitions ---------------------------------------------*/

void HilManager_ProcessCommand(void)
{
  HilCommandPacket_t cmd;
  while (tud_vendor_n_available(VENDOR_ITF_HIL_CONTROL) >= CMD_SIZE) {
    uint32_t count = tud_vendor_n_read(VENDOR_ITF_HIL_CONTROL, &cmd, CMD_SIZE);
    if (count == CMD_SIZE) processHilCommand(&cmd);
  }
}

void HilManager_CalibrationDone(void)
{
  HilCalibrationPacket_t response_pkt;
  if (HilCal_Get(&response_pkt) == false) {
    REGISTER_ERROR(ERROR_HIL_CAL);
    return;
  }

  response_pkt.response_id = HIL_RESPONSE_CALIBRATION;
  response_pkt.adc_bits = FEEDBACK_ADC_BITS;
  response_pkt.dac_bits = 12;
  response_pkt.adc_sampling_rate = HIL_SAMPLING_RATE;
  response_pkt.dac_sampling_rate = HIL_SAMPLING_RATE;

  // ADC and DAC are both single-ended around mid-rail with the analog supply
  // as the reference; peak swing from mid-rail equals VDDA / 2. Sample 1.0
  // (normalized) corresponds to V_ref_peak above mid-rail at the pin. ADC_VREF
  // is defined in mess_filt_resources.h; the DAC shares the same supply.
  response_pkt.adc_vref_peak_volts = ADC_VREF / 2.0f;
  response_pkt.dac_vref_peak_volts = ADC_VREF / 2.0f;

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &response_pkt, 
      sizeof(HilCalibrationPacket_t)) != sizeof(HilCalibrationPacket_t))
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);

  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}

HilState_t HilManager_HilMode(void)
{
  return hil_state;
}

void HilManager_SetState(HilState_t new_state)
{
  switch (new_state) {
    case HIL_STATE_RX:
      enterHilRxMode();
      break;
    case HIL_STATE_TX:
      enterHilTxMode();
      break;
    case HIL_STATE_TRANSITIONING:
      enterHilTransitionMode();
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
  sendStateTransitionPacket();
}

void HilManager_SendUpdate(uint16_t buffer_fill, uint16_t buffer_size, uint16_t packet_id)
{
  HilStatus_t status_pkt = {0};
  status_pkt.response_id = HIL_RESPONSE_STATUS;
  status_pkt.state = hil_state;
  status_pkt.buffer_fill = buffer_fill;
  status_pkt.buffer_capacity = buffer_size;
  status_pkt.attenuation_index = 1;
  status_pkt.error_flags = HilBuf_ReadAndClearErrorFlags();
  status_pkt.timestamp_ms = HAL_GetTick();
  status_pkt.packet_id = packet_id;
  // Sample next_packet_index as late as possible to minimize the gap between
  // stamp time and EP2 IN transmission time
  status_pkt.next_packet_index = HilBuf_GetNextPacketIndex();

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &status_pkt, RESPONSE_SIZE) != RESPONSE_SIZE)
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}

/* Private function definitions ----------------------------------------------*/

static void processHilCommand(const HilCommandPacket_t* cmd)
{
  switch (cmd->cmd_id) {
    case HIL_CMD_START_HIL_CAL:
      osEventFlagsSet(print_event_handle, MESS_HIL_CAL_START);
      break;
    case HIL_CMD_START_HIL:
      enterHilMode();
      break;
    case HIL_CMD_QUIT:
      exitHilMode();
      break;
    case HIL_CMD_PING:
      pingRespond();
      break;
    case HIL_CMD_RESET:
      // Debug-only. resetHil() stops DMA, so we must also drop state to
      // IDLE to keep hil_state and peripheral state in sync; otherwise the
      // state-gated callbacks would still expect DMA to be running.
      exitHilMode();
      break;
    default:
      REGISTER_ERROR(ERROR_HIL_CMD_UNKNOWN);
  }
}

static void pingRespond(void)
{
  HilCalibrationPacket_t response = {0};
  response.response_id = HIL_RESPONSE_PING_RESPONSE;

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &response, RESPONSE_SIZE) != RESPONSE_SIZE)
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}

static void enterHilMode(void)
{
  if (Waveform_IsRunning() == true) {
    Waveform_StopWaveformOutput();
  }
  resetHil();
  hil_state = HIL_STATE_RX;
  // Defer the DAC start until the host has prefilled the ring. Otherwise the
  // DAC drains an empty buffer for ~30 s before USB throughput catches up,
  // producing chronic underruns through the entire startup transient.
  HilBuf_ArmDeferredDacStart();

  osEventFlagsSet(print_event_handle, MESS_HIL_START);
}

static void exitHilMode(void)
{
  if (Waveform_IsRunning() == true) {
    Waveform_StopWaveformOutput();
  }
  resetHil();
  hil_state = HIL_STATE_IDLE;

  osEventFlagsSet(print_event_handle, MESS_HIL_STOP);
}

static void resetHil(void)
{
  HilStream_StopAdc();
  if (Waveform_IsRunning() == false) {
    HilStream_StopDac();
  }
  HilBuf_Reset();
  tud_vendor_n_read_flush(VENDOR_ITF_HIL_STREAM);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_STREAM);
  osThreadFlagsClear(HIL_EVT_ADC_HALF_FULL | HIL_EVT_ADC_FULL |
                     HIL_EVT_DAC_HALF_FULL | HIL_EVT_DAC_FULL |
                     HIL_EVT_STREAM_TX_CPLT | HIL_EVT_STREAM_RX_RDY |
                     HIL_EVT_CONTROL_CMD | HIL_EVT_ENTER_RX |
                     HIL_EVT_ENTER_TX | HIL_EVT_ENTER_TRANSITION);
}

static void enterHilRxMode(void)
{
  resetHil();
  hil_state = HIL_STATE_RX;
  HilBuf_ArmDeferredDacStart();
}

static void enterHilTxMode(void)
{
  resetHil();
  hil_state = HIL_STATE_TX;
  HilStream_StartAdc();
}

static void enterHilTransitionMode(void)
{
  resetHil();
  hil_state = HIL_STATE_TRANSITIONING;
}

static void sendStateTransitionPacket(void)
{
  // Host only consumes `state` from this packet; everything else is
  // zero-initialized so no stack garbage is leaked in `reserved[]`.
  HilStatus_t status_pkt = {0};
  status_pkt.response_id = HIL_RESPONSE_STATUS;
  status_pkt.state = hil_state;
  status_pkt.buffer_capacity = 16384;
  status_pkt.attenuation_index = 1;
  status_pkt.timestamp_ms = HAL_GetTick();

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &status_pkt, RESPONSE_SIZE) != RESPONSE_SIZE)
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}
