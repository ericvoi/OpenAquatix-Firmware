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
  // Others as needed...
  NUM_HOPPERS
} FhbfskHopperMethod_t;

// Struct for all configuration parameters that are relevant for feedback tests
// Other configuration parameters belong to modules
// IMPORTANT: Any modification to parameters here must be reflected in the feedback tests
typedef struct {
  float baud_rate;
  ModDemodMethod_t mod_demod_method;
  uint32_t fsk_f0;
  uint32_t fsk_f1;
  uint32_t fc;
  uint8_t fhbfsk_freq_spacing;
  uint8_t fhbfsk_num_tones;
  uint8_t fhbfsk_dwell_time;
  ErrorDetectionMethod_t error_detection_method;
  ErrorCorrectionMethod_t ecc_method_preamble;
  ErrorCorrectionMethod_t ecc_method_message;
  bool use_interleaver;
  FhbfskHopperMethod_t fhbfsk_hopper;
} DspConfig_t;


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_DSP_CONFIG_H_ */
