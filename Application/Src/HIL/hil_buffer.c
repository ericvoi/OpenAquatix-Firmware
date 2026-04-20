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

#ifndef HIL_WRAP_TRACE
#define HIL_WRAP_TRACE 1
#endif

#if HIL_WRAP_TRACE
#include "hmi_usb.h"
#include <stdio.h>
#endif

/* Private typedef -----------------------------------------------------------*/

#define SAMPLES_PER_RX_PACKET                       (255U)
#define SAMPLES_PER_TX_PACKET                       (255U)

#define STATUS_PACKET_EVERY                         (7U) // Send a status packet every x packets

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

// Defer DAC start until the ring has at least this many samples buffered.
// Matches the host pacer target_fill (0.5 of capacity) so the system enters
// steady state immediately rather than ramping up through an underrun zone.
#define DAC_PREFILL_THRESHOLD         (RING_BUFFER_SIZE / 2)

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

// True between HilBuf_ArmDeferredDacStart() and the moment the ring crosses
// DAC_PREFILL_THRESHOLD, at which point HilBuf_ReadRxPackets() starts the DAC
// and clears this flag. Cleared unconditionally by HilBuf_Reset().
static bool dac_start_pending = false;

// Diagnostic: one-shot log of next_packet_index vs first incoming rx_pi at the
// start of each RX session. Set by HilBuf_Reset, cleared after the dump.
// Tells us definitively whether the host or firmware starts misaligned when
// BAD_PACKET_INDEX accumulates ~(2^16 - drift) at startup.
static bool log_first_rx_packet = false;

/* Private function prototypes -----------------------------------------------*/

static uint16_t availableSamples(void);

/* ------------------------- HIL_WRAP_TRACE -------------------------------- */
/*
 * Debug instrumentation for the uint16 packet-index wrap. Captures every
 * read-loop iteration and underrun event when next_packet_index is within
 * WRAP_TRACE_WINDOW of the wrap boundary (0xFFFF -> 0x0000), then emits a
 * tab-separated dump over the CDC HMI interface once the window closes.
 *
 * Gate with `#define HIL_WRAP_TRACE 0` in the build to compile out.
 */
#if HIL_WRAP_TRACE

#define WRAP_TRACE_LO          (0xFFF0U)  // enter window when nPi >= this
#define WRAP_TRACE_HI          (0x001FU)  // ... or nPi <= this
#define WRAP_TRACE_CAPACITY    (192U)

typedef enum {
  WRAP_EV_ACCEPTED       = 'A',  // Packet accepted, counter advanced
  WRAP_EV_INDEX_MISMATCH = 'I',  // rx_packet.packet_index != next_packet_index
  WRAP_EV_RING_FULL      = 'F',  // TryAddPacket failed (ring full)
  WRAP_EV_SHORT_READ     = 'S',  // tud_vendor_n_read returned < 512
  WRAP_EV_UNDERRUN       = 'U',  // GetData found available < requested
} WrapTraceEv_t;

typedef struct {
  uint32_t t_ms;       // HAL_GetTick() at record time
  uint16_t n_pi;       // next_packet_index at record time (before/after, per event)
  uint16_t rx_pi;      // rx_packet.packet_index (or 0 for UNDERRUN)
  uint16_t usb_avail;  // tud_vendor_n_available() snapshot (or 0 for UNDERRUN)
  uint32_t n_read;     // bytes returned by tud_vendor_n_read (or samples req for UNDERRUN)
  uint16_t ring_fill;  // availableSamples() at record time
  uint8_t  errs;       // hil_error_flags snapshot
  char     ev;         // WrapTraceEv_t
} WrapTraceRec_t;

static WrapTraceRec_t trace_buf[WRAP_TRACE_CAPACITY];
static uint16_t trace_count = 0;
static bool     trace_prev_in_window = false;
static uint32_t trace_seq = 0;  // dump sequence number; #1 is startup, #2+ are wraps

static inline bool inWrapWindow(uint16_t n_pi)
{
  return (n_pi >= WRAP_TRACE_LO) || (n_pi <= WRAP_TRACE_HI);
}

static void wrapTraceRecord(uint16_t n_pi, uint16_t rx_pi, uint16_t usb_avail,
                            uint32_t n_read, uint16_t ring_fill, uint8_t errs,
                            WrapTraceEv_t ev)
{
  if (!inWrapWindow(n_pi)) return;
  if (trace_count >= WRAP_TRACE_CAPACITY) return;
  WrapTraceRec_t* r = &trace_buf[trace_count++];
  r->t_ms      = HAL_GetTick();
  r->n_pi      = n_pi;
  r->rx_pi     = rx_pi;
  r->usb_avail = usb_avail;
  r->n_read    = n_read;
  r->ring_fill = ring_fill;
  r->errs      = errs;
  r->ev        = (char) ev;
}

static void wrapTraceDump(void)
{
  char line[96];
  trace_seq++;
  int n = snprintf(line, sizeof(line),
                   "\r\n--- HIL trace #%lu (%u recs) ---\r\n"
                   "t_ms\tnPi\trxPi\tusbAv\tnRd\tring\terr\tev\r\n",
                   (unsigned long) trace_seq, (unsigned) trace_count);
  if (n > 0) USB_TransmitData((uint8_t*) line, (uint16_t) n);

  for (uint16_t i = 0; i < trace_count; i++) {
    const WrapTraceRec_t* r = &trace_buf[i];
    n = snprintf(line, sizeof(line),
                 "%lu\t%04X\t%04X\t%u\t%lu\t%u\t%02X\t%c\r\n",
                 (unsigned long) r->t_ms,
                 (unsigned) r->n_pi,
                 (unsigned) r->rx_pi,
                 (unsigned) r->usb_avail,
                 (unsigned long) r->n_read,
                 (unsigned) r->ring_fill,
                 (unsigned) r->errs,
                 r->ev);
    if (n > 0) USB_TransmitData((uint8_t*) line, (uint16_t) n);
  }

  n = snprintf(line, sizeof(line), "--- end trace ---\r\n");
  if (n > 0) USB_TransmitData((uint8_t*) line, (uint16_t) n);
}

// Called from the RX path after every completed accept (next_packet_index
// just advanced). Handles arm-on-entry and dump-on-exit of the wrap window.
static void wrapTraceStep(uint16_t n_pi_after)
{
  bool now_in_window = inWrapWindow(n_pi_after);
  if (now_in_window && !trace_prev_in_window) {
    trace_count = 0;  // re-arm on entering window
  }
  if (!now_in_window && trace_prev_in_window && trace_count > 0) {
    wrapTraceDump();
    trace_count = 0;
  }
  trace_prev_in_window = now_in_window;
}

#define WRAP_TRACE_REC(n_pi, rx_pi, usb_avail, n_read, ring_fill, errs, ev) \
  wrapTraceRecord((n_pi), (rx_pi), (usb_avail), (n_read), (ring_fill), (errs), (ev))
#define WRAP_TRACE_STEP(n_pi_after) wrapTraceStep(n_pi_after)

#else  /* HIL_WRAP_TRACE */

#define WRAP_TRACE_REC(...)  ((void) 0)
#define WRAP_TRACE_STEP(...) ((void) 0)

#endif /* HIL_WRAP_TRACE */

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
    WRAP_TRACE_REC(next_packet_index, 0, 0, samples, available_samples,
                   (uint8_t) (hil_error_flags | 0x01), WRAP_EV_UNDERRUN);
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
    uint16_t usb_avail = (uint16_t) tud_vendor_n_available(VENDOR_ITF_HIL_STREAM);
    uint32_t n = tud_vendor_n_read(VENDOR_ITF_HIL_STREAM, &rx_packet, sizeof(HilRxData_t));

#if HIL_WRAP_TRACE
    if (log_first_rx_packet) {
      log_first_rx_packet = false;
      char line[96];
      int written = snprintf(line, sizeof(line),
                             "\r\n[HIL] First RX pkt: rx_pi=0x%04X expected=0x%04X usb_av=%u\r\n",
                             (unsigned) rx_packet.packet_index,
                             (unsigned) next_packet_index,
                             (unsigned) usb_avail);
      if (written > 0) USB_TransmitData((uint8_t*) line, (uint16_t) written);
    }
#endif

    if (n != sizeof(HilRxData_t)) {
      WRAP_TRACE_REC(next_packet_index, rx_packet.packet_index, usb_avail, n,
                     availableSamples(), hil_error_flags, WRAP_EV_SHORT_READ);
      REGISTER_ERROR(ERROR_HIL_SHORT_READ);
      break;
    }

    if (rx_packet.packet_index != next_packet_index) {
      WRAP_TRACE_REC(next_packet_index, rx_packet.packet_index, usb_avail, n,
                     availableSamples(), hil_error_flags, WRAP_EV_INDEX_MISMATCH);
      // Spec §3.2: bad index → do not accept, do not advance counter.
      REGISTER_ERROR(ERROR_HIL_BAD_PACKET_INDEX);
      continue;
    }

    if (!HilBuf_TryAddPacket(rx_packet.data, SAMPLES_PER_RX_PACKET)) {
      WRAP_TRACE_REC(next_packet_index, rx_packet.packet_index, usb_avail, n,
                     availableSamples(), hil_error_flags, WRAP_EV_RING_FULL);
      REGISTER_ERROR(ERROR_HIL_RING_FULL);
      // Ring full → drop whole packet, leave counter in place so host sees drift.
      continue;
    }

    next_packet_index++;

    WRAP_TRACE_STEP(next_packet_index);
    WRAP_TRACE_REC(next_packet_index, rx_packet.packet_index, usb_avail, n,
                   availableSamples(), hil_error_flags, WRAP_EV_ACCEPTED);

    if (dac_start_pending && availableSamples() >= DAC_PREFILL_THRESHOLD) {
      HilStream_StartDac();
      dac_start_pending = false;
    }

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
  dac_start_pending = false;
  log_first_rx_packet = true;
#if HIL_WRAP_TRACE
  trace_count = 0;
  trace_prev_in_window = false;
  // Note: trace_seq is intentionally NOT reset, so dumps stay numbered
  // across resets and you can tell whether a resetHil fired between dumps.
#endif
}

void HilBuf_ArmDeferredDacStart(void)
{
  dac_start_pending = true;
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
