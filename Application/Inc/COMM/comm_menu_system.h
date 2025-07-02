/*
 * comm_menu_system.h
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

#ifndef __COMM_MENU_SYSTEM_H_
#define __COMM_MENU_SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "comm_main.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MENU_ID_MAIN = 0,             // Main menu with links to all other menus
  MENU_ID_CFG,                  // Configuration menu for waveform processing, daughter card, and LED
  MENU_ID_DBG,                  // Debug menu
  MENU_ID_HIST,                 // Menu with historical messages sent and received as well as power and temp
  MENU_ID_TXRX,                 // Transmission/reception menu
  MENU_ID_EVAL,                 // Evaluation option menu
  MENU_ID_CFG_UNIV,             // Universal waveform porcessing parameters
  MENU_ID_CFG_UNIV_ERR,         // Error detection parameters
  MENU_ID_CFG_UNIV_ERR_PREAMBLE,// Preamble error detection method
  MENU_ID_CFG_UNIV_ERR_CARGO,   // Cargo error detection method
  MENU_ID_CFG_UNIV_ERR_PREERR,  // What to do when the preamble has an error
  MENU_ID_CFG_UNIV_ERR_CARGOERR,// What to do when the cargo has an error
  MENU_ID_CFG_UNIV_ECCPREAMBLE, // ECC to use on the preamble
  MENU_ID_CFG_UNIV_ECCMESSAGE,  // ECC to use on the data 
  MENU_ID_CFG_UNIV_MOD,         // Modulation scheme used for both reception and transmission
  MENU_ID_CFG_UNIV_FSK,         // FSK based waveform processing parameters
  MENU_ID_CFG_UNIV_FSK_F0,      // FSK frequency corresponding to bit 0
  MENU_ID_CFG_UNIV_FSK_F1,      // FSK frequency corresponding to bit 1
  MENU_ID_CFG_UNIV_FHBFSK,      // FHBFSK based waveform processing parameters
  MENU_ID_CFG_UNIV_FHBFSK_FSEP, // Integer frequency separation to use in the FHBFSK scheme
  MENU_ID_CFG_UNIV_FHBFSK_DWELL,// Number of bit periods to dwell on a tone in FHBFSK
  MENU_ID_CFG_UNIV_FHBFSK_TONES,// Number of tones to use in the FHBFSK modulations scheme
  MENU_ID_CFG_UNIV_FHBFSK_HOPP, // Frequency hopping method to use
  MENU_ID_CFG_UNIV_BAUD,        // Raw baud rate used for transmission
  MENU_ID_CFG_UNIV_FC,          // Center frequency used 
  MENU_ID_CFG_UNIV_BP,          // Bit period used in the baud rate. Currently the inverse of ^^
  MENU_ID_CFG_UNIV_BANDWIDTH,   // Bandwidth
  MENU_ID_CFG_UNIV_INTERLEAVER, // Whether to use the JANUS interleaver
  MENU_ID_CFG_UNIV_SYNC,        // Synchronization sequence to use
  MENU_ID_CFG_UNIV_EXP,         // Export the configuration options used
  MENU_ID_CFG_UNIV_IMP,         // Import configuration options
  MENU_ID_CFG_MOD,              // Waveform modulation parameters
  MENU_ID_CFG_MOD_TLEN,         // Maximum code change to use with the DAC
  MENU_ID_CFG_MOD_CAL,          // Calibration menu for the modem's modulation
  MENU_ID_CFG_MOD_CAL_LOWFREQ,  // Lower frequency for calibration
  MENU_ID_CFG_MOD_CAL_HIFREQ,   // Upper frequency for calibration
  MENU_ID_CFG_MOD_CAL_TVR,      // Import the TVR of the hydrophone being used
  MENU_ID_CFG_MOD_CAL_PERFORM,  // Perform a calibration
  MENU_ID_CFG_MOD_CAL_EXP,      // Export calibration data
  MENU_ID_CFG_MOD_CAL_TUNE,     // Tune the matching network (adjusting capacitors in network)
  MENU_ID_CFG_MOD_CAL_RECV,     // Import the OCRR of the hydrophone being used
  MENU_ID_CFG_MOD_CAL_VMAX,     // Maximum output voltage allowable
  MENU_ID_CFG_MOD_FB,           // Modulation feedback network options
  MENU_ID_CFG_MOD_FB_EN,        // Enable or disable the feedback network (saves CPU cycles)
  MENU_ID_CFG_MOD_FB_RATIO,     // Voltage division ratio used in the feedback network
  MENU_ID_CFG_MOD_METHOD,       // Method to control output power
  MENU_ID_CFG_MOD_FIXED,        // Fixed relative DAC output strength to use
  MENU_ID_CFG_MOD_PWROPT,       // Fixed output power level options
  MENU_ID_CFG_MOD_PWROPT_PWR,   // Target output power to aim for
  MENU_ID_CFG_MOD_PWROPT_R,     // Series resistance of the motional branch
  MENU_ID_CFG_MOD_PWROPT_C0,    // Series capacitance of the motional branch
  MENU_ID_CFG_MOD_PWROPT_L0,    // Series inductance of the motional branch
  MENU_ID_CFG_MOD_PWROPT_C1,    // Parallel capacitance with the motional branch
  MENU_ID_CFG_DEMOD,            // Waveform demodulation parameters
  MENU_ID_CFG_DEMOD_CAL,        // Calibration options for demodulation
  MENU_ID_CFG_DEMOD_CAL_RATIO,  // Voltage division ratio on the feedback network
  MENU_ID_CFG_DEMOD_CAL_PERFORM,// Perform calibration on input amplifier
  MENU_ID_CFG_DEMOD_CAL_LOWFREQ,// Lower frequency used for demodulation calibration
  MENU_ID_CFG_DEMOD_CAL_HIFREQ, // Upper frequency used for demodulation calibration
  MENU_ID_CFG_DEMOD_CAL_EXP,    // Export calibration results
  MENU_ID_CFG_DEMOD_START,      // Select the message start function to use
  MENU_ID_CFG_DEMOD_DECISION,   // Select the bit decision maker
  MENU_ID_CFG_DEMOD_CMPTHRESH,  // Historical comparison threshold
  MENU_ID_CFG_DEMOD_AGCEN,      // Enable/disable automatic gain control (AGC)
  MENU_ID_CFG_DEMOD_GAIN,       // Set fixed PGA gain
  MENU_ID_CFG_DAU,              // Daughter card configuration options
  MENU_ID_CFG_DAU_SLEEP,        // Enable/disable sleep modes from the daughter card
  MENU_ID_CFG_LED,              // LED configuration options
  MENU_ID_CFG_LED_BRIGHTNESS,   // Set the brightness of the onboard LED
  MENU_ID_CFG_LED_EN,           // Enable/disable the onboard LED
  MENU_ID_CFG_SETID,            // Set the ID to be used for transmission
  MENU_ID_CFG_STATIONARY,       // Whether the modem is stationary or not
  MENU_ID_DBG_GPIO,             // Dump the state of all used GPIO inputs and outputs
  MENU_ID_DBG_SETLED,           // Set the colour of the onboard LED
  MENU_ID_DBG_PRINT,            // Print the next received waveform when it is received
  MENU_ID_DBG_BGDUMP,           // 1000 sample ADC dump
  MENU_ID_DBG_BGFREQ,           // Frequency content of background noise
  MENU_ID_DBG_TEMP,             // Current temperature
  MENU_ID_DBG_ERR,              // Current errors
  MENU_ID_DBG_PWR,              // Current power consumption
  MENU_ID_DBG_DFU,              // Enter DFU mode to flash new firmware over USB
  MENU_ID_DBG_RESETCONFIG,      // Reset saved configuration 
  MENU_ID_HIST_PWR,             // History of power
  MENU_ID_HIST_PWR_PEAK,        // Peak power consumption since boot
  MENU_ID_HIST_PWR_BOOT,        // Total power consumption since boot
  MENU_ID_HIST_PWR_AVG,         // Average power consumption since boot
  MENU_ID_HIST_PWR_CURR,        // Current power consumption
  MENU_ID_HIST_RECV,            // Print the last 5 received messages
  MENU_ID_HIST_SENT,            // Print the last 5 sent messages
  MENU_ID_HIST_ERR,             // Error log since boot
  MENU_ID_HIST_TEMP,            // History of device temperatures
  MENU_ID_HIST_TEMP_CURR,       // Current device temeprature
  MENU_ID_HIST_TEMP_PEAK,       // Peak device temeprature
  MENU_ID_HIST_TEMP_AVG,        // Average device temeprature
  MENU_ID_TXRX_BITSOUT,         // Transmit a series of bits through transducer
  MENU_ID_TXRX_BITSFB,          // Send bits through the feedback network
  MENU_ID_TXRX_STROUT,          // Transmit a string through transducer
  MENU_ID_TXRX_STRFB,           // Transmit a string through the feedback network
  MENU_ID_TXRX_INTOUT,          // Transmit an integer through transducer
  MENU_ID_TXRX_INTFB,           // Transmit an integer through feedback network
  MENU_ID_TXRX_FLOATOUT,        // Transmit a float through transducer
  MENU_ID_TXRX_FLOATFB,         // Transmit a float through feedback
  MENU_ID_TXRX_ENPNT,           // Enable/disable printing of waveforms as they are received
  MENU_ID_EVAL_TOGGLE,          // Toggle evaluation mode
  MENU_ID_EVAL_SETMSG,          // Set the message to compare to
  MENU_ID_EVAL_FEEDBACK,        // Send evaluation message through feedback network
  MENU_ID_EVAL_TRANSDUCER,      // Send evaluation message through transducer
  MENU_ID_EVAL_FEEDBACKTESTS,   // Performs the feedback network tests
  // ... other menu IDs can be added freely at any location
  MENU_ID_COUNT
} MenuID_t;

typedef enum {
  PARAM_STATE_0,
  PARAM_STATE_1,
  PARAM_STATE_2,
  PARAM_STATE_3,
  PARAM_STATE_4,
  PARAM_STATE_5,
  PARAM_STATE_6,
 // ... other parameter states as needed
  PARAM_STATE_COMPLETE
} ParamState_t;

typedef struct {
  ParamState_t state;
  uint16_t param_id;
} ParamContext_t;

typedef struct MenuNode {
  uint16_t id;
  uint64_t alt_id;
  const char* description;
  void (*handler)(void* param);
  MenuID_t parent_id;
  MenuID_t* children_ids;
  uint8_t num_children;
  uint8_t access_level;
  ParamContext_t* parameters;
} MenuNode_t;

typedef struct {
  ParamContext_t* state;
  char input[MAX_COMM_IN_BUFFER_SIZE];
  uint16_t input_len;
  uint8_t *output_buffer;
  CommInterface_t comm_interface;
} FunctionContext_t;


/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Registers a menu in the system menu registry
 *
 * Stores the menu in the global menu registry indexed by its ID.
 * Each menu ID can only have one registered menu at a time.
 *
 * @param menu Pointer to the menu to register. Must have a valid ID.
 *
 * @return true if menu was successfully registered,
 *         false if the menu ID is invalid or already in use
 *
 * @note This function casts away const when storing the menu pointer
 */
bool registerMenu(const MenuNode_t* menu);

/**
 * @brief Retrieves a menu from the system menu registry
 *
 * @param id The ID of the menu to retrieve
 *
 * @return Pointer to the requested menu if found, NULL otherwise
 */
MenuNode_t* getMenu(MenuID_t id);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __COMM_MENU_SYSTEM_H_ */
