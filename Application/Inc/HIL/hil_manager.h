/*
 * hil_manager.h
 *
 *  Created on: Apr 3, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef HIL_HIL_MANAGER_H_
#define HIL_HIL_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  HIL_STATE_IDLE = 0U,
  HIL_STATE_RX = 1U,
  HIL_STATE_TX = 2U,
  HIL_STATE_TRANSITIONING = 3U
} HilState_t;

typedef enum {
  HIL_CMD_START_HIL_CAL = 0x10,
  HIL_CMD_START_HIL = 0x11,
  HIL_CMD_QUIT = 0x12,
  HIL_CMD_SET_ATTENUATION = 0x13,

  HIL_CMD_PING,
  HIL_CMD_RESET,
} HilCommands_t;

typedef enum {
  HIL_RESPONSE_STATUS = 0x01,
  HIL_RESPONSE_CALIBRATION = 0x02,
  HIL_RESPONSE_PING_RESPONSE
} HilResponses_t;

typedef struct __attribute__((packed)) {
  HilCommands_t cmd_id;
  uint8_t attenuation;
  uint8_t reserved[62];
} HilCommandPacket_t;

// Calibration packet — must remain byte-identical to the host's
// CalibrationPayload (src/protocol/packets.hpp). 64 bytes total, packed.
//
// Field offsets (kept naturally aligned within the packed struct):
//   [ 0]  response_id (1 with -fshort-enums)
//   [ 1]  adc_bits, dac_bits, num_input_attenuations           (3 bytes)
//   [ 4]  loopback_cal_attenuation                             (uint16)
//   [ 6]  _reserved0                                           (uint16, alignment)
//   [ 8]  noise_floor_psd_counts_per_sqrt_hz                   (float)
//   [12]  loopback_gain                                        (float)
//   [16]  adc_sampling_rate                                    (uint32)
//   [20]  dac_sampling_rate                                    (uint32)
//   [24]  input_attenuation[2]                                 (2 × float)
//   [32]  output_attenuation                                   (float)
//   [36]  center_freq_hz                                       (float)
//   [40]  adc_vref_peak_volts                                  (float)
//   [44]  dac_vref_peak_volts                                  (float)
//   [48]  reserved[16]                                         (16 bytes)
typedef struct __attribute__((packed)) {
  HilResponses_t response_id;
  uint8_t adc_bits;
  uint8_t dac_bits;
  uint8_t num_input_attenuations;
  uint16_t loopback_cal_attenuation;
  uint16_t _reserved0;
  float noise_floor_psd_counts_per_sqrt_hz;
  float loopback_gain;
  uint32_t adc_sampling_rate;
  uint32_t dac_sampling_rate;
  float input_attenuation[2];
  float output_attenuation;
  float center_freq_hz;
  float adc_vref_peak_volts;
  float dac_vref_peak_volts;
  uint8_t reserved[16];
} HilCalibrationPacket_t;

typedef struct __attribute__((packed)) {
  HilResponses_t response_id; // HIL_RESPONSE_STATUS = 0x01
  HilState_t state;
  uint16_t buffer_fill;
  uint16_t buffer_capacity;
  uint8_t attenuation_index;
  uint8_t error_flags;
  uint32_t timestamp_ms;
  uint16_t next_packet_index;
  uint16_t packet_id;
  uint8_t reserved[48];
} HilStatus_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Processes commands received on HIL control interface and calls
 * appropriate function.
 */
void HilManager_ProcessCommand(void);

/**
 * @brief Reads calibration data and sends the packet to the host
 * 
 * @note This can only be called after the modem has calibrated successfully
 */
void HilManager_CalibrationDone(void);

/**
 * @brief Returns true if HIL not idle
 * 
 * @return HilState_t HIL_STATE_IDLE - Not in any HIL mode
 *                    HIL_STATE_RX - Actively driving feedback from host data
 *                    HIL_STATE_TX - Actively sending tx feedback data to host
 *                    HIL_STATE_TRANSITIONING - Transitioning between HIL states. Neither consuming nor producing samples
 */
HilState_t HilManager_HilMode(void);

/**
 * @brief Sets the HIL state. Handles the toggling of peripherals
 * 
 * @param new_state New HIL state
 */
void HilManager_SetState(HilState_t new_state);

/**
 * @brief Sends status update packet to the host to assist with buffer fill
 * pacing.
 *
 * `next_packet_index` is sampled from the ring buffer counter inside this
 * function (as late as possible before the USB write) so the reported value
 * reflects state closer to transmission, per spec §3.2 capture-timing note.
 *
 * @param buffer_fill The number of samples in the buffer
 * @param buffer_size The total buffer capacity (in samples)
 * @param packet_id The packet id whose arrival triggered this status (fill_reference_id)
 */
void HilManager_SendUpdate(uint16_t buffer_fill, uint16_t buffer_size,
                           uint16_t packet_id);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* HIL_HIL_MANAGER_H_ */
