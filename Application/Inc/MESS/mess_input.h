/*
 * mess_input.h
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef MESS_MESS_INPUT_H_
#define MESS_MESS_INPUT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

#include "mess_packet.h"
#include "mess_main.h"
#include "mess_sync.h"
#include "mess_dsp_config.h"
#include "mess_sync.h"
#include "cfg_parameters.h"

#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define MESSAGE_START_FUNCTION_TABLE(X) \
  X(MSG_START_AMPLITUDE, "Absolute amplitude threshold") \
  X(MSG_START_FREQUENCY, "Frequency amplitude threshold")

#define PREAMBLE_ERROR_BEHAVIOR_TABLE(X) \
  X(PREAMBLE_ERROR_DROP, "Drop packet silently") \
  X(PREAMBLE_ERROR_DECODE, "Attempt to decode the cargo") \
  X(PREAMBLE_ERROR_NOTIFY, "Drop the message and notify the user")

DECLARE_ENUM(MESSAGE_START_FUNCTION_TABLE, NUM_MSG_START_FCN, MsgStartFunctions_t)
DECLARE_ENUM(PREAMBLE_ERROR_BEHAVIOR_TABLE, NUM_PREAMBLE_ERROR_BEHAVIORS, PreambleErrorBehavior_t)

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the input processing module
 *
 * Sets up circular buffer indices, initializes analysis blocks, registers
 * the input buffer with the ADC, and initializes the FFT engine.
 *
 * @return true if initialization succeeds, false if ADC registration or FFT initialization fails
 *
 * @note Must be called before any other Input_* functions
 */
void Input_Init();

/**
 * @brief Detects the start of an acoustic message in the input stream
 *
 * Applies the currently configured detection method (amplitude or frequency-based)
 * to determine if a valid message transmission has begun.
 * 
 * @param cfg DSP configuration defining how to detect message start
 * @param msg Message to add synchronization info to
 * @param sync_state (Modified) SYNC_SUCCESS if synchronized, SYNC_OK otherwise
 */
void Input_DetectMessageStart(const DspConfig_t* cfg, Message_t* msg, SyncState_t* sync_state);

/**
 * @brief Segments input buffer into analysis blocks for demodulation
 *
 * Creates analysis blocks from the input data stream based on the current baud rate.
 * Each block contains data needed to demodulate one bit of the message.
 * 
 * @param cfg DSP configuration defining how to segment blocks
 */
void Input_SegmentBlocks(const DspConfig_t* cfg);

/**
 * @brief Processes analysis blocks to extract bits from the received signal
 *
 * Demodulates each pending analysis block, extracting the bit value and
 * storing demodulation metrics for evaluation purposes.
 *
 * @param bit_msg Pointer to the bit message structure where decoded bits are stored
 * @param eval_info Pointer to evaluation metrics structure to record signal quality data
 *
 * @warning Potential for eval_info overflow - needs to be addressed
 */
void Input_ProcessBlocks(BitMessage_t* bit_msg, const DspConfig_t* cfg);

/**
 * @brief Decodes header information from accumulated bits
 *
 * Attempts to extract message header fields (sender ID, data type, length, etc.)
 * once sufficient bits have been received.
 *
 * @param bit_msg Pointer to the bit message structure containing received bits
 * @param cfg Pointer to configuration data
 * @param msg Message to add extracted bits to
 * @param proceed Whether to continue decoding message or not failure
 */
void Input_DecodeBits(BitMessage_t* bit_msg, const DspConfig_t* cfg, Message_t* msg, bool* proceed);

/**
 * @brief Resets the input module to initial state
 *
 * Clears all buffer indices, analysis state, and input buffer memory.
 * Used to prepare for receiving a new message.
 */
void Input_Reset();

/**
 * @brief Reconfgures the parameters used in the input module
 * 
 * @param cfg Pointer to new configuration data
 */
void Input_Reconfigure(const DspConfig_t* cfg);

/**
 * @brief Transmits current buffer data over USB for noise analysis
 *
 * Sends the entire input buffer content via USB in chunks, with appropriate
 * thread synchronization via semaphores.
 *
 * @note This function blocks while transmitting data
 */
void Input_PrintNoise();

/**
 * @brief Prints waveform as it is received
 * 
 * Looks for any new received data that has not been printed and prints it
 * over USB ONLY. This function must be called with a script as the data rates
 * (~2 Mbps) are excessive for a terminal emulator
 * 
 * @param print_next_waveform Whether the next waveform shoould be printed.
 * Note that the function changes this to false to terminate when it is done
 * @param fully_received Whether the message being decoded has been fully received
 */
void Input_PrintWaveform(bool* print_next_waveform, bool fully_received);

/**
 * @brief Performs noise analysis on the input with 128-point FFTs. Averages
 * the results and then displays them
 */
void Input_NoiseFft();

/**
 * @brief Dummy function for AGC
 */
void Input_UpdatePgaGain();

/**
 * @brief Registers module parameters with the parameter system
 *
 * Makes the message start function parameter accessible via the HMI interface.
 *
 * @note Logs an error in the event of a failure
 */
void Input_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_INPUT_H_ */
