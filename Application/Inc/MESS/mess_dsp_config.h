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
#include "mess_main.h"
#include "mess_error_detection.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  float baud_rate;
  ModDemodMethod_t mod_demod_method;
  uint32_t fsk_f0;
  uint32_t fsk_f1;
  uint32_t fc;
  uint8_t fhbfsk_freq_spacing;
  uint8_t fhbfsk_num_tones;
  uint8_t fhbfsk_dwell_time;
  ErrorDetectionMethod_t error_correction_method;
} DspConfig_t;


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_DSP_CONFIG_H_ */
