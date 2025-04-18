/*
 * cfg_defaults.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

#ifndef CFG_CFG_DEFAULTS_H_
#define CFG_CFG_DEFAULTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_input.h"
#include "mess_main.h"
#include "mess_error_correction.h"
#include "mess_demodulate.h"
#include "mess_modulate.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define DEFAULT_BAUD_RATE           (100.0f)
#define MIN_BAUD_RATE               (10.0f)
#define MAX_BAUD_RATE               (1000.0f)

#define DEFAULT_OUTPUT_AMPLITUDE    (0.1f)
#define MIN_OUTPUT_AMPLITUDE        (0.02f)
#define MAX_OUTPUT_AMPLITUDE        (0.5f)

#define DEFAULT_MSG_START_FCN       (MSG_START_FREQUENCY)
#define MIN_MSG_START_FCN           0
#define MAX_MSG_START_FCN           (NUM_MSG_START_FCN - 1)

#define DEFAULT_FSK_F0              29000
#define DEFAULT_FSK_F1              30000
#define MIN_FSK_FREQUENCY           25000
#define MAX_FSK_FREQUENCY           38000

#define DEFAULT_MOD_DEMOD_METHOD    (MOD_DEMOD_FSK)
#define MIN_MOD_DEMOD_METHOD        0
#define MAX_MOD_DEMOD_METHOD        (NUM_MOD_DEMOD_METHODS - 1)

#define DEFAULT_FC                  31500
#define MIN_FC                      30000
#define MAX_FC                      33000

#define DEFAULT_BANDWIDTH           4000
#define MIN_BANDWIDTH               1000
#define MAX_BANDWIDTH               6000

#define DEFAULT_FHBFSK_FREQ_SPACING 1
#define MIN_FHBFSK_FREQ_SPACING     1
#define MAX_FHBFSK_FREQ_SPACING     10

#define DEFAULT_FHBFSK_DWELL_TIME   1
#define MIN_FHBFSK_DWELL_TIME       1
#define MAX_FHBFSK_DWELL_TIME       4

#define DEFAULT_PRINT_ENABLED       (true)
#define MIN_PRINT_ENABLED           (false)
#define MAX_PRINT_ENABLED           (true)

#define DEFAULT_FHBFSK_NUM_TONES    5
#define MIN_FHBFSK_NUM_TONES        2
#define MAX_FHBFSK_NUM_TONES        30

#define DEFAULT_EVAL_MODE_STATE     (false)
#define MIN_EVAL_MODE_STATE         (false)
#define MAX_EVAL_MODE_STATE         (true)

#define DEFAULT_EVAL_MESSAGE        1
#define MIN_EVAL_MESSAGE            1
#define MAX_EVAL_MESSAGE            5

#define DEFAULT_ID                  2
#define MIN_ID                      0
#define MAX_ID                      15 // 2^4 -1

#define DEFAULT_STATIONARY_FLAG     (true)
#define MIN_STATIONARY_FLAG         (false)
#define MAX_STATIONARY_FLAG         (true)

#define DEFAULT_ERROR_CORRECTION    (CRC_16)
#define MIN_ERROR_CORRECTION        0
#define MAX_ERROR_CORRECTION        (NUM_ERROR_CORRECTION_METHODS - 1)

#define DEFAULT_DEMOD_DECISION      (HISTORICAL_COMPARISON)
#define MIN_DEMOD_DECISION          0
#define MAX_DEMOD_DECISION          (NUM_DEMODULATION_DECISION - 1)

#define DEFAULT_DAC_TRANSITION_LEN  64
#define MIN_DAC_TRANSITION_LEN      8
#define MAX_DAC_TRANSITION_LEN      1000

#define DEFAULT_MOD_OUTPUT_METHOD   (MOD_OUTPUT_STATIC_DAC)
#define MIN_MOD_OUTPUT_METHOD       0
#define MAX_MOD_OUTPUT_METHOD       (NUM_MOD_OUTPUT_LEVEL_CONTROL - 1)

#define DEFAULT_MOD_TARGET_POWER    (0.5f)
#define MIN_MOD_TARGET_POWER        (0.005f)
#define MAX_MOD_TARGET_POWER        (5.0f)

#define DEFAULT_R                   (300.0f)
#define MIN_R                       (50.0f)
#define MAX_R                       (1000.0f)

#define DEFAULT_C0                  (1.91f)
#define MIN_C0                      (0.1f)
#define MAX_C0                      (50.0f)

#define DEFAULT_L0                  (13.3f)
#define MIN_L0                      (0.5f)
#define MAX_L0                      (400.0f)

#define DEFAULT_C1                  (20.0f)
#define MIN_C1                      (0.5f)
#define MAX_C1                      (300.0f)

#define DEFAULT_MOD_CAL_LOWER_FREQ  28000
#define MIN_MOD_CAL_LOWER_FREQ      15000
#define MAX_MOD_CAL_LOWER_FREQ      90000

#define DEFAULT_MOD_CAL_UPPER_FREQ  36000
#define MIN_MOD_CAL_UPPER_FREQ      20000
#define MAX_MOD_CAL_UPPER_FREQ      100000

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CFG_CFG_DEFAULTS_H_ */
