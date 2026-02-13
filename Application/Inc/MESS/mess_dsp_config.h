/*
 * mess_dsp_config.h
 *
 *  Created on: Apr 21, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_DSP_CONFIG_H_
#define MESS_MESS_DSP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "cfg_parameters.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define MOD_DEMOD_METHODS_TABLE(X) \
  X(MOD_DEMOD_FSK, "FSK") \
  X(MOD_DEMOD_FHBFSK, "FH-BFSK")

#define ERROR_DETECTION_METHOD_TABLE(X) \
  X(NO_ERROR_DETECTION, "No error detection") \
  X(CRC_8, "CRC-8") \
  X(CRC_16, "CRC-16") \
  X(CRC_32, "CRC-32") \
  X(CHECKSUM_8, "Checksum-8") \
  X(CHECKSUM_16, "Checksum-16") \
  X(CHECKSUM_32, "Checksum_32") 

#define ERROR_CORRECTION_METHOD_TABLE(X) \
  X(NO_ECC, "No error correction") \
  X(HAMMING_CODE, "1-bit Hamming code") \
  X(JANUS_CONVOLUTIONAL, "1:2 Convolutional Code (JANUS)")

#define FHBFSK_HOPPER_TABLE(X) \
  X(HOPPER_INCREMENT, "Increment by 1") \
  X(HOPPER_GALOIS, "Galois Field arithmetic (JANUS)") \
  X(HOPPER_PRIME, "Prime selector")

#define SYNCHRONIZATION_METHOD_TABLE(X) \
  X(NO_SYNC, "None") \
  X(SYNC_PN_32_JANUS, "JANUS 32-chips")

#define MESSAGING_PROTOCOL_TABLE(X) \
  X(PROTOCOL_CUSTOM, "Custom protocol") \
  X(PROTOCOL_JANUS, "JANUS protocol")

DECLARE_ENUM(MOD_DEMOD_METHODS_TABLE, NUM_MOD_DEMOD_METHODS, ModDemodMethod_t)
DECLARE_ENUM(ERROR_DETECTION_METHOD_TABLE, NUM_ERROR_DETECTION_METHODS, ErrorDetectionMethod_t)
DECLARE_ENUM(ERROR_CORRECTION_METHOD_TABLE, NUM_ECC_METHODS, ErrorCorrectionMethod_t)
DECLARE_ENUM(FHBFSK_HOPPER_TABLE, NUM_HOPPERS, FhbfskHopperMethod_t)
DECLARE_ENUM(SYNCHRONIZATION_METHOD_TABLE, NUM_SYNC_METHODS, SynchronizationMethod_t)
DECLARE_ENUM(MESSAGING_PROTOCOL_TABLE, NUM_PROTOCOLS, MessagingProtocol_t)

// Preamble structs
typedef struct {
  uint16_t value;
  bool valid;
} PreambleValue_t;

typedef struct preamble_content {
  PreambleValue_t modem_id;
  PreambleValue_t message_type;
  PreambleValue_t is_mobile;
  PreambleValue_t cargo_length;
  PreambleValue_t tx_rx_capable;
  PreambleValue_t can_forward;
  PreambleValue_t coding;
  PreambleValue_t encryption;
  PreambleValue_t destination_id;
  PreambleValue_t reservation_time_10ms; // Pre-divided by 10ms to fit within uint16
  PreambleValue_t class_user_id;
  PreambleValue_t application_type;
  PreambleValue_t schedule_flag;
  // others as needed
} PreambleContent_t;

// Struct for all configuration parameters that are relevant for feedback tests
// Other configuration parameters belong to modules
// IMPORTANT: Any modification to parameters here must be reflected in the feedback tests
typedef struct {
  float baud_rate;                              // b/s
  ModDemodMethod_t mod_demod_method;            // How to encode information
  uint32_t fsk_f0;                              // Frequency corresponding to 0 in FSK
  uint32_t fsk_f1;                              // Frequency corresponding to 1 in FSK
  uint32_t fc;                                  // Center frequency for FH-BFSK
  uint8_t fhbfsk_freq_spacing;                  // Integer spacing between adjacent frequencies in FH-BFSK
  uint8_t fhbfsk_num_tones;                     // Number of FH-BFSK tone pairs
  uint8_t fhbfsk_dwell_time;                    // Number of symbols to remain on a FH-bFSK tone pair
  ErrorDetectionMethod_t preamble_validation;   // Error detection method to use on the message preamble
  ErrorDetectionMethod_t cargo_validation;      // Error detection method to use on the message cargo
  ErrorCorrectionMethod_t preamble_ecc_method;  // Error correction method to use on the message preamble
  ErrorCorrectionMethod_t cargo_ecc_method;     // Error correction method to use on the message cargo
  bool use_interleaver;                         // Whether to interleave the bits in each section with JANUS interleaver
  FhbfskHopperMethod_t fhbfsk_hopper;           // Decides which tone pair to use in FH-BFSK
  SynchronizationMethod_t sync_method;          // Synchronization method to use between transmitter and receiver
  bool wakeup_tones;                            // Whether to precede messages with a series of wakeup tones
  uint32_t wakeup_tone1;                        // First of three wakeup tones
  uint32_t wakeup_tone2;                        // Second of three wakeup tones
  uint32_t wakeup_tone3;                        // Third of three wakeup tones
  MessagingProtocol_t protocol;                 // Messaging protocol defining preambles and cargo contents
} DspConfig_t;


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_DSP_CONFIG_H_ */
