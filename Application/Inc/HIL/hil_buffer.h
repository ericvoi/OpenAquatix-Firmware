/*
 * hil_buffer.h
 *
 *  Created on: Apr 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef HIL_HIL_BUFFER_H_
#define HIL_HIL_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds data to the HIL ring buffer (partial-copy on overrun).
 *
 * Intended for the ADC DMA path, where dropping a whole half-buffer would
 * create audible gaps. On overrun, copies what fits and latches the TX-overrun
 * sticky error flag.
 *
 * @param src Source array to take the data from
 * @param src_len Length of the source array
 */
void HilBuf_AddData(volatile const uint16_t* src, uint16_t src_len);

/**
 * @brief Atomically enqueue a whole RX packet, or drop it.
 *
 * Intended for the USB RX path. If there is not enough room for @p src_len
 * samples, nothing is enqueued and the function returns false. The caller
 * must not advance the RX expected-packet counter when this returns false.
 *
 * @return true  iff all samples were enqueued
 * @return false iff the ring could not fit the packet (TX-overrun flag is set)
 */
bool HilBuf_TryAddPacket(const uint16_t* src, uint16_t src_len);

/**
 * @brief Gets data from the HIL ring buffer. Adds a mid-value to fill in gaps
 * 
 * @param dst Destination array (modified)
 * @param samples The number of samples to take
 * @param mid_value In case there are not enough samples in the ring buffer, 
 * use this value 
 */
void HilBuf_GetData(volatile uint16_t* dst, uint16_t samples, uint16_t mid_value);

/**
 * @brief Checks the HIL stream endpoint for any new rx packets. Drains the rx
 * stream of any packets
 */
void HilBuf_ReadRxPackets(void);

/**
 * @brief Checks buffer to see if a new tx packet can be sent. If so, sends as
 * many as possible
 */
void HilBuf_SendTxPackets(void);

/**
 * @brief Resets the HIL ring buffer by setting all entries to 0 and resetting
 * head/tail. Also clears the sticky error flags and zeroes the RX/TX packet
 * index counter. Cancels any pending deferred DAC start.
 */
void HilBuf_Reset(void);

/**
 * @brief Arm the deferred DAC start. Once armed, HilBuf_ReadRxPackets() will
 * start the DAC (HilStream_StartDac) automatically once the ring fill reaches
 * the prefill threshold. Use instead of calling HilStream_StartDac() directly
 * on RX entry — prevents the long startup transient where DAC drains an empty
 * ring before the host pacer can keep up.
 */
void HilBuf_ArmDeferredDacStart(void);

/**
 * @brief Returns the current RX/TX packet-index counter (rx_expected_id when
 * in RX; next outgoing packet_id when in TX).
 */
uint16_t HilBuf_GetNextPacketIndex(void);

/**
 * @brief Latch-and-clear the sticky error flags (bit 0 = RX underrun, bit 1 = 
 * TX overrun). Called from the status-packet emitter so each status report 
 * covers only the interval since the previous one.
 */
uint8_t HilBuf_ReadAndClearErrorFlags(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_BUFFER_H_ */
