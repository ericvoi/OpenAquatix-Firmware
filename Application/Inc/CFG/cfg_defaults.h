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
#include "mess_error_detection.h"
#include "mess_demodulate.h"
#include "mess_modulate.h"
#include "mess_error_correction.h"
#include "mess_demodulate.h"

#include "pga113-driver.h"

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

#define DEFAULT_EVAL_MESSAGE_LEN    100
#define MIN_EVAL_MESSAGE_LEN        1
#define MAX_EVAL_MESSAGE_LEN        480

#define DEFAULT_ID                  2
#define MIN_ID                      0
#define MAX_ID                      15 // 2^4 -1

#define DEFAULT_STATIONARY_FLAG     (true)
#define MIN_STATIONARY_FLAG         (false)
#define MAX_STATIONARY_FLAG         (true)

#define DEFAULT_PREAMBLE_ERROR_DETECTION  (CRC_8)
#define DEFAULT_CARGO_ERROR_DETECTION     (CRC_16)
#define MIN_ERROR_DETECTION         0
#define MAX_ERROR_DETECTION         (NUM_ERROR_DETECTION_METHODS - 1)

#define DEFAULT_DEMOD_DECISION      (HISTORICAL_COMPARISON)
#define MIN_DEMOD_DECISION          0
#define MAX_DEMOD_DECISION          (NUM_DEMODULATION_DECISION - 1)

#define DEFAULT_DAC_TRANSITION_LEN  64
#define MIN_DAC_TRANSITION_LEN      8
#define MAX_DAC_TRANSITION_LEN      249

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

#define DEFAULT_MAX_TRANSDUCER_V    (80.0f)
#define MIN_MAX_TRANSDUCER_V        (10.0f)
#define MAX_MAX_TRANSDUCER_V        (87.0f)

#define DEFAULT_DEMOD_CAL_LOWER_F   28000
#define MIN_DEMOD_CAL_LOWER_F       15000
#define MAX_DEMOD_CAL_LOWER_F       90000

#define DEFAULT_DEMOD_CAL_UPPER_F   36000
#define MIN_DEMOD_CAL_UPPER_F       20000
#define MAX_DEMOD_CAL_UPPER_F       100000

#define DEFAULT_HIST_CMP_THRESH     (0.25f)
#define MIN_HIST_CMP_THRESH         (0.05f)
#define MAX_HIST_CMP_THRESH         (0.5f)

#define DEFAULT_LED_BRIGHTNESS      20
#define MIN_LED_BRIGHTNESS          1
#define MAX_LED_BRIGHTNESS          255

#define DEFAULT_LED_STATE           (true)
#define MIN_LED_STATE               (false)
#define MAX_LED_STATE               (true)

#define DEFAULT_AGC_STATE           (false)
#define MIN_AGC_STATE               (false)
#define MAX_AGC_STATE               (true)

#define DEFAULT_FIXED_PGA_GAIN      (PGA_GAIN_1)
#define MIN_FIXED_PGA_GAIN          0
#define MAX_FIXED_PGA_GAIN          (PGA_NUM_CODES - 1)

#define DEFAULT_ECC_PREAMBLE        (HAMMING_CODE)
#define DEFAULT_ECC_MESSAGE         (HAMMING_CODE)
#define MIN_ECC_METHOD              0
#define MAX_ECC_METHOD              (NUM_ECC_METHODS - 1)

#define DEFAULT_INTERLEAVER_STATE   (false)
#define MIN_INTERLEAVER_STATE       (false)
#define MAX_INTERLEAVER_STATE       (true)

#define DEFAULT_FHBFSK_HOPPER       (HOPPER_GALOIS)
#define MIN_FHBFSK_HOPPER           0
#define MAX_FHBFSK_HOPPER           (NUM_HOPPERS - 1)

#define DEFAULT_SYNC_METHOD         (NO_SYNC)
#define MIN_SYNC_METHOD             0
#define MAX_SYNC_METHOD             (NUM_SYNC_METHODS - 1)

#define DEFAULT_WINDOW_FUNCTION     (WINDOW_HANN)
#define MIN_WINDOW_FUNCTION         0
#define MAX_WINDOW_FUNCTION         (NUM_WINDOW_FUNCTIONS - 1)

#define DEFAULT_WAKEUP_TONES_STATE  (false)
#define MIN_WAKEUP_TONES_STATE      (false)
#define MAX_WAKEUP_TONES_STATE      (true)

#define DEFAULT_WAKEUP_TONE_FREQ1   (27000U)
#define DEFAULT_WAKEUP_TONE_FREQ2   (30000U)
#define DEFAULT_WAKEUP_TONE_FREQ3   (33000U)
#define MIN_WAKEUP_TONE_FREQ        (20000U)
#define MAX_WAKEUP_TONE_FREQ        (40000U)

#define DEFAULT_MESSAGING_PROTOCOL  (PROTOCOL_CUSTOM)
#define MIN_MESSAGING_PROTOCOL      (0)
#define MAX_MESSAGING_PROTOCOL      (NUM_PROTOCOLS - 1)

#define TX_ONLY                     (false)
#define BOTH_TX_RX                  (true)
#define DEFAULT_TX_RX_CAPABLE       (BOTH_TX_RX)
#define MIN_TX_RX_CAPABLE           (0)
#define MAX_TX_RX_CAPABLE           (1)

#define DEFAULT_FORWARD_CAPABILITY  (false)
#define MIN_FORWARD_CAPABILITY      (false)
#define MAX_FORWARD_CAPABILITY      (true)

#define DEFAULT_JANUS_ID            (73U)
#define MIN_JANUS_ID                (0U)
#define MAX_JANUS_ID                (254U)

#define DEFAULT_JANUS_DESTINATION   (73U)
#define MIN_JANUS_DESTINATION       (0U)
#define MAX_JANUS_DESTINATION       (255U)

#define DEFAULT_CODING              (CODING_ASCII8)
#define MIN_CODING                  (0)
#define MAX_CODING                  (NUM_CODING_METHODS - 1)

#define DEFAULT_ENCRYPTION          (ENCRYPTION_NONE)
#define MIN_ENCRYPTION              (0)
#define MAX_ENCRYPTION              (NUM_ENCRYPTION_METHODS - 1)



// JANUS basic parameters
#define JANUS_MOD_DEMOD             (MOD_DEMOD_FHBFSK)
#define JANUS_FHBFSK_FREQ_SPACING   (1)
#define JANUS_FHBFSK_NUM_TONES      (13)
#define JANUS_FHBFSK_DWELL_TIME     (1)
#define JANUS_PREAMBLE_VALIDATION   (CRC_8)
#define JANUS_CARGO_VALIDATION      (CRC_16)
#define JANUS_PREAMBLE_ECC          (JANUS_CONVOLUTIONAL)
#define JANUS_CARGO_ECC             (JANUS_CONVOLUTIONAL)
#define JANUS_INTERLEAVER           (true)
#define JANUS_HOPPER                (HOPPER_GALOIS)
#define JANUS_SYNC_METHOD           (SYNC_PN_32_JANUS)

// #define JANUS_BAND_A
// #define JANUS_BAND_B
// #define JANUS_BAND_C
// #define JANUS_BAND_D
#define JANUS_BAND_E

#ifdef JANUS_BAND_A
#define JANUS_FC                    (11520U)
#define JANUS_BAUD                  (160U)
#else
#ifdef JANUS_BAND_B
#define JANUS_FC                    (6000U)
#define JANUS_BAUD                  (80U)
#else
#ifdef JANUS_BAND_C
#define JANUS_FC                    (9700U)
#define JANUS_BAUD                  (100U)
#else
#ifdef JANUS_BAND_D
#define JANUS_FC                    (14080U)
#define JANUS_BAUD                  (160U)
#else
#ifdef JANUS_BAND_E
#define JANUS_FC                    (28000U)
#define JANUS_BAUD                  (250U)
#else
# error "Please specify a JANUS band to use"
#endif
#endif
#endif
#endif
#endif

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CFG_CFG_DEFAULTS_H_ */
