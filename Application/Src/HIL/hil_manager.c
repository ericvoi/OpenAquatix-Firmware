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

void HilManager_SendUpdate(uint16_t buffer_fill, uint16_t buffer_size, uint16_t packet_id, uint16_t next_packet_index)
{
  HilStatus_t status_pkt;
  status_pkt.response_id = HIL_RESPONSE_STATUS;
  status_pkt.state = hil_state;
  status_pkt.buffer_fill = buffer_fill;
  status_pkt.buffer_capacity = buffer_size;
  status_pkt.attenuation_index = 1;
  status_pkt.error_flags = 0;
  status_pkt.timestamp_ms = HAL_GetTick();
  status_pkt.packet_id = packet_id;
  status_pkt.next_packet_index = next_packet_index;

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
      resetHil();
      break;
    default:
      REGISTER_ERROR(ERROR_HIL_CMD_UNKNOWN);
  }
}

static void pingRespond(void)
{
  HilCalibrationPacket_t response;
  response.response_id = HIL_RESPONSE_PING_RESPONSE;

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &response, RESPONSE_SIZE) != RESPONSE_SIZE)
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}

static void enterHilMode(void)
{
  hil_state = HIL_STATE_RX;
  resetHil();
  HilStream_StopAdc();
  HilStream_StopDac();

  HilStream_StartDac();

  osEventFlagsSet(print_event_handle, MESS_HIL_START);
}

static void exitHilMode(void)
{
  hil_state = HIL_STATE_IDLE;
  resetHil();
  HilStream_StopAdc();
  HilStream_StopDac();

  osEventFlagsSet(print_event_handle, MESS_HIL_STOP);
}

static void resetHil(void)
{
  HilBuf_Reset();
  tud_vendor_n_read_flush(VENDOR_ITF_HIL_STREAM);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_STREAM);
}

static void enterHilRxMode(void)
{
  hil_state = HIL_STATE_RX;
  resetHil();

  HilStream_StopAdc();
  HilStream_StopDac();

  HilStream_StartDac();
}

static void enterHilTxMode(void)
{
  hil_state = HIL_STATE_TX;
  resetHil();

  HilStream_StopAdc();
  HilStream_StopDac();

  HilStream_StartAdc();
}

static void enterHilTransitionMode(void)
{
  hil_state = HIL_STATE_TRANSITIONING;
  resetHil();

  HilStream_StopAdc();
  HilStream_StopDac();
}

static void sendStateTransitionPacket(void)
{
  HilStatus_t status_pkt;
  status_pkt.response_id = HIL_RESPONSE_STATUS;
  status_pkt.state = hil_state;
  status_pkt.buffer_fill = 0;
  status_pkt.buffer_capacity = 16384; // TODO
  status_pkt.attenuation_index = 1; // TODO
  status_pkt.error_flags = 0; // TODO
  status_pkt.timestamp_ms = HAL_GetTick();

  if (tud_vendor_n_write(VENDOR_ITF_HIL_CONTROL, &status_pkt, RESPONSE_SIZE) != RESPONSE_SIZE)
    REGISTER_ERROR(ERROR_HIL_RESPONSE_SEND_FAIL);
  tud_vendor_n_write_flush(VENDOR_ITF_HIL_CONTROL);
}
