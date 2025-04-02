/*
 * mess_input.h
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
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

#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MSG_START_AMPLITUDE,
  MSG_START_FREQUENCY,
  NUM_MSG_START_FCN
} MsgStartFunctions_t;

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
bool Input_Init();

/**
 * @brief Increments the buffer end index after new ADC data has been received
 *
 * Updates the circular buffer end pointer to incorporate newly received samples.
 *
 * @warning Does not currently check for buffer overflow conditions
 */
void Input_IncrementEndIndex();

/**
 * @brief Detects the start of an acoustic message in the input stream
 *
 * Applies the currently configured detection method (amplitude or frequency-based)
 * to determine if a valid message transmission has begun.
 *
 * @return true if a message start is detected, false otherwise
 */
bool Input_DetectMessageStart();

/**
 * @brief Segments input buffer into analysis blocks for demodulation
 *
 * Creates analysis blocks from the input data stream based on the current baud rate.
 * Each block contains data needed to demodulate one bit of the message.
 *
 * @return true if segmentation succeeds, false if analysis buffer capacity is exceeded
 */
bool Input_SegmentBlocks();

/**
 * @brief Processes analysis blocks to extract bits from the received signal
 *
 * Demodulates each pending analysis block, extracting the bit value and
 * storing demodulation metrics for evaluation purposes.
 *
 * @param bit_msg Pointer to the bit message structure where decoded bits are stored
 * @param eval_info Pointer to evaluation metrics structure to record signal quality data
 *
 * @return true if processing succeeds, false on parameter error or processing failure
 *
 * @warning Potential for eval_info overflow - needs to be addressed
 */
bool Input_ProcessBlocks(BitMessage_t* bit_msg, EvalMessageInfo_t* eval_info);

/**
 * @brief Decodes header information from accumulated bits
 *
 * Attempts to extract message header fields (sender ID, data type, length, etc.)
 * once sufficient bits have been received.
 *
 * @param bit_msg Pointer to the bit message structure containing received bits
 * @param evaluation_mode If true, bypasses header decoding (no header in evaluation mode)
 *
 * @return true if decoding succeeds or is not yet needed, false on decoding failure
 */
bool Input_DecodeBits(BitMessage_t* bit_msg, bool evaluation_mode);

/**
 * @brief Extracts payload data from bit message into a structured message
 *
 * Converts the stream of bits in the bit message into bytes and copies them
 * to the message data field.
 *
 * @param input_bit_msg Pointer to the bit message containing the encoded data
 * @param msg Pointer to message structure where decoded data will be stored
 *
 * @return true if message extraction succeeds, false on parameter error or extraction failure
 *
 * @pre Preamble must have been successfully received and decoded
 */
bool Input_DecodeMessage(BitMessage_t* input_bit_msg, Message_t* msg);

/**
 * @brief Resets the input module to initial state
 *
 * Clears all buffer indices, analysis state, and input buffer memory.
 * Used to prepare for receiving a new message.
 */
void Input_Reset();

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
 * @return true if processing
 */
bool Input_PrintWaveform(bool* print_next_waveform, bool fully_received);

/**
 * @brief Registers module parameters with the parameter system
 *
 * Makes the message start function parameter accessible via the HMI interface.
 *
 * @return true if parameter registration succeeds, false otherwise
 */
bool Input_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_INPUT_H_ */
