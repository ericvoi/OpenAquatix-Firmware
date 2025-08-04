/*
 * mess_dsp_config.h
 *
 *  Created on: Apr 21, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_DSP_CONFIG_H_
#define MESS_MESS_DSP_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

// Preamble structs
typedef struct {
  uint16_t value;
  bool valid;
} PreambleValue_t;

typedef struct preamble_content {
  PreambleValue_t modem_id;
  PreambleValue_t message_type;
  PreambleValue_t is_stationary;
  PreambleValue_t cargo_length;
  // others as needed
} PreambleContent_t;

// Parameter enums
typedef enum {
  MOD_DEMOD_FSK,
  MOD_DEMOD_FHBFSK,
  // Place others as needed here
  NUM_MOD_DEMOD_METHODS
} ModDemodMethod_t;

typedef enum {
  NO_ERROR_DETECTION,
  CRC_8,
  CRC_16,
  CRC_32,
  CHECKSUM_8,
  CHECKSUM_16,
  CHECKSUM_32,
  // Place others as needed here
  NUM_ERROR_DETECTION_METHODS
} ErrorDetectionMethod_t;

typedef enum {
  NO_ECC,
  HAMMING_CODE,
  JANUS_CONVOLUTIONAL,
  // Place others as needed here
  NUM_ECC_METHODS
} ErrorCorrectionMethod_t;

typedef enum {
  HOPPER_INCREMENT,
  HOPPER_GALOIS,
  HOPPER_PRIME,
  // Place others as needed here
  NUM_HOPPERS
} FhbfskHopperMethod_t;

typedef enum {
  NO_SYNC,
  SYNC_PN_32_JANUS,
  // Place others as needed here
  NUM_SYNC_METHODS
} SynchronizationMethod_t;

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
} DspConfig_t;


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_DSP_CONFIG_H_ */
