/*
 * hil_buffer.c
 *
 *  Created on: Apr 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "hil_buffer.h"
#include "hil_main.h"
#include "hil_stream.h"
#include "hil_manager.h"
#include "tusb.h"
#include "usb_main.h"
#include "error_manager.h"
#include "mess_filt_resources.h"
#include <stdint.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

#define SAMPLES_PER_RX_PACKET                       (255U)
#define SAMPLES_PER_TX_PACKET                       (255U)

#define STATUS_PACKET_EVERY                         (5U) // Send a status packet every x packets

// From the host to the modem
typedef struct {
  uint16_t packet_index;
  uint16_t data[SAMPLES_PER_RX_PACKET];
} HilRxData_t;

// From the modem to the host
typedef struct {
  uint16_t packet_index;
  uint16_t data[SAMPLES_PER_TX_PACKET];
} HilTxData_t;

_Static_assert((sizeof(HilRxData_t) == USB_HS_PACKET_SIZE), "Rx data packets must be 512 bytes");
_Static_assert((sizeof(HilTxData_t) == USB_HS_PACKET_SIZE), "Tx data packets must be 512 bytes");

/* Private define ------------------------------------------------------------*/

#define RING_BUFFER_SIZE              (1 << 14) // 16384
#define RING_BUFFER_MASK              (RING_BUFFER_SIZE - 1)

/* Private macro -------------------------------------------------------------*/

#define MIN(x, y) ((x < y) ? (x) : (y))

/* Private variables ---------------------------------------------------------*/

static uint16_t tx_rx_ring_buf[RING_BUFFER_SIZE];
static uint16_t ring_buf_head = 0;
static uint16_t ring_buf_tail = 0;
static uint16_t next_packet_index = 0;

// bit 0 = RX underrun, bit 1 = TX overrun.
// Latch-and-clear on every status transmission.
static volatile uint8_t hil_error_flags = 0;

/* Private function prototypes -----------------------------------------------*/

static uint16_t availableSamples(void);

/* Exported function definitions ---------------------------------------------*/

void HilBuf_AddData(volatile const uint16_t* src, uint16_t src_len)
{
  if (src == NULL) REGISTER_ERROR(ERROR_NULL_PTR);

  uint16_t buf_room = RING_BUFFER_SIZE - availableSamples();

  if (src_len > buf_room) {
    hil_error_flags |= 0x02; // bit 1: TX overrun
    REGISTER_ERROR(ERROR_HIL_BUF_OVERRUN);
  }

  uint16_t samples_to_add = MIN(buf_room, src_len);

  for (uint16_t i = 0; i < samples_to_add; i++) {
    tx_rx_ring_buf[ring_buf_head++] = src[i];
    ring_buf_head &= RING_BUFFER_MASK;
  }
}

bool HilBuf_TryAddPacket(const uint16_t* src, uint16_t src_len)
{
  if (src == NULL) REGISTER_ERROR_NON_VOID(ERROR_NULL_PTR, false);

  uint16_t buf_room = RING_BUFFER_SIZE - availableSamples();

  if (src_len > buf_room) {
    hil_error_flags |= 0x02; // bit 1: TX overrun (RX ring full)
    // On drop, the caller must NOT advance rx_expected_id. Return
    // without touching ring_buf_head so the packet is atomically rejected.
    return false;
  }

  for (uint16_t i = 0; i < src_len; i++) {
    tx_rx_ring_buf[ring_buf_head++] = src[i];
    ring_buf_head &= RING_BUFFER_MASK;
  }
  return true;
}

void HilBuf_GetData(volatile uint16_t* dst, uint16_t samples, uint16_t mid_value)
{
  if (dst == NULL) REGISTER_ERROR(ERROR_NULL_PTR);

  uint16_t available_samples = availableSamples();

  if (available_samples < samples) {
    hil_error_flags |= 0x01; // bit 0: RX underrun
    REGISTER_ERROR(ERROR_HIL_BUF_UNDERRUN);
    for (uint16_t i = 0; i < samples; i++) {
      if (i >= available_samples) {
        dst[i] = mid_value;
      }
      else {
        dst[i] = tx_rx_ring_buf[ring_buf_tail++];
        ring_buf_tail &= RING_BUFFER_MASK;
      }
    }
  }
  else {
    for (uint16_t i = 0; i < samples; i++) {
      dst[i] = tx_rx_ring_buf[ring_buf_tail++];
      ring_buf_tail &= RING_BUFFER_MASK;
    }
  }
}

void HilBuf_ReadRxPackets(void)
{
  if (HilManager_HilMode() != HIL_STATE_RX) return;

  HilRxData_t rx_packet;
  while (tud_vendor_n_available(VENDOR_ITF_HIL_STREAM) >= sizeof(HilRxData_t)) {
    uint32_t n = tud_vendor_n_read(VENDOR_ITF_HIL_STREAM, &rx_packet, sizeof(HilRxData_t));
    if (n != sizeof(HilRxData_t)) {
      REGISTER_ERROR(ERROR_HIL_BAD_PACKET_INDEX);
      break;
    }

    if (rx_packet.packet_index != next_packet_index) {
      // Spec §3.2: bad index → do not accept, do not advance counter.
      REGISTER_ERROR(ERROR_HIL_BAD_PACKET_INDEX);
      continue;
    }

    if (!HilBuf_TryAddPacket(rx_packet.data, SAMPLES_PER_RX_PACKET)) {
      // Ring full → drop whole packet, leave counter in place so host sees drift.
      continue;
    }

    next_packet_index++;

    if ((next_packet_index % STATUS_PACKET_EVERY) == 0) {
      HilManager_SendUpdate(availableSamples(), RING_BUFFER_SIZE, rx_packet.packet_index);
    }
  }
}

uint32_t cb_cnt1 = 0;
uint32_t cb_cnt2 = 0;

void HilBuf_SendTxPackets(void)
{
  if (HilManager_HilMode() != HIL_STATE_TX) return;

  HilTxData_t tx_packet;
//  if ((availableSamples() >= SAMPLES_PER_TX_PACKET) && (tud_vendor_n_write_available(VENDOR_ITF_HIL_STREAM) < sizeof(HilTxData_t))) {
//    __BKPT(0);
//  }
  cb_cnt1++;
  while ((availableSamples() >= SAMPLES_PER_TX_PACKET) &&
         (tud_vendor_n_write_available(VENDOR_ITF_HIL_STREAM) >= sizeof(HilTxData_t))) {
    cb_cnt2++;
    tx_packet.packet_index = next_packet_index++;

    HilBuf_GetData(tx_packet.data, SAMPLES_PER_TX_PACKET, 1 << (FEEDBACK_ADC_BITS - 1));

    tud_vendor_n_write(VENDOR_ITF_HIL_STREAM, &tx_packet, sizeof(HilTxData_t));
    tud_vendor_n_write_flush(VENDOR_ITF_HIL_STREAM);
  }
}

void HilBuf_Reset(void)
{
  memset(tx_rx_ring_buf, 0, sizeof(uint16_t) * RING_BUFFER_SIZE);
  ring_buf_head = 0;
  ring_buf_tail = 0;
  next_packet_index = 0;
  hil_error_flags = 0;
}

uint16_t HilBuf_GetNextPacketIndex(void)
{
  return next_packet_index;
}

uint8_t HilBuf_ReadAndClearErrorFlags(void)
{
  uint8_t f = hil_error_flags;
  hil_error_flags = 0;
  return f;
}

/* Private function definitions ----------------------------------------------*/

static uint16_t availableSamples(void)
{
  if (ring_buf_head == ring_buf_tail) return 0;
  return (ring_buf_head - ring_buf_tail) & RING_BUFFER_MASK;
}
