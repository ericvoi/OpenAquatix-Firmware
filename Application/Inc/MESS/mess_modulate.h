/*
 * mess_modulate.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_MODULATE_H_
#define MESS_MESS_MODULATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include "dac_waveform.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MOD_OUTPUT_STATIC_DAC,
  MOD_OUTPUT_STATIC_PWR,
  // Others as needed...
  NUM_MOD_OUTPUT_LEVEL_CONTROL
} OutputStrengthMethod_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Converts bit message to frequency sequence using the configured modulation method
 *
 * Maps bits to appropriate frequencies according to the selected modulation scheme
 * (FSK or FHBFSK) by delegating to the corresponding conversion function.
 *
 * @param bit_msg Pointer to bit message structure to be converted
 * @param message_sequence Pointer to output waveform step array to store the result
 *
 * @return true if conversion was successful, false otherwise
 */
bool Modulate_ConvertToFrequency(BitMessage_t* bit_msg, WaveformStep_t* message_sequence);

/**
 * @brief Applies the configured amplitude to all waveform steps in the sequence
 *
 * Sets the relative_amplitude field of each waveform step to the global output_amplitude value.
 *
 * @param message_sequence Pointer to waveform step array
 * @param len Number of steps in the sequence
 *
 * @return true if operation was successful
 */
bool Modulate_ApplyAmplitude(WaveformStep_t* message_sequence, uint16_t len);

/**
 * @brief Sets duration for all waveform steps based on the configured baud rate
 *
 * Calculates step duration in microseconds as 1,000,000/baud_rate and applies
 * it to each waveform step in the sequence.
 *
 * @param message_sequence Pointer to waveform step array
 * @param len Number of steps in the sequence
 *
 * @return true if operation was successful
 */
bool Modulate_ApplyDuration(WaveformStep_t* message_sequence, uint16_t len);

/**
 * @brief Returns the current transducer output amplitude setting
 *
 * @return Current amplitude value
 */
float Modulate_GetTransducerAmplitude(void);

/**
 * @brief Updates the transducer output amplitude setting
 *
 * @param new_amplitude New amplitude value to be applied
 */
void Modulate_ChangeTransducerAmplitude(float new_amplitude);

/**
 * @brief Initializes and starts the transducer output subsystem
 *
 * Performs a controlled sequence of shutting down active peripherals,
 * starting ADC feedback, enabling DAC waveform output, and starting
 * the timer. Includes necessary delays for peripheral stabilization.
 *
 * @return true if all peripherals started successfully, false otherwise
 */
bool Modulate_StartTransducerOutput();

/**
 * @brief Generates a simple two-frequency test sequence for transducer testing
 *
 * Creates a test pattern alternating between 30kHz and 33kHz with 1ms durations.
 * Uses the currently configured output amplitude.
 */
void Modulate_TestOutput();

/**
 * @brief Sets the frequency to be used for frequency response testing
 *
 * @param freq_hz Test frequency in Hertz
 */
void Modulate_SetTestFrequency(uint32_t freq_hz);

/**
 * @brief Generates a single-frequency test signal for frequency response analysis
 *
 * Creates a test waveform at the previously configured test frequency for
 * FEEDBACK_TEST_DURATION_MS milliseconds. Uses the current output amplitude setting.
 */
void Modulate_TestFrequencyResponse();

/**
 * @brief Calculates the frequency for a given bit using FHBFSK modulation
 *
 * Implements Frequency-Hopped Binary FSK modulation by determining the appropriate
 * frequency for the given bit value and position. The frequency hops according to
 * a pattern determined by the bit index and configured dwell time.
 *
 * @param bit The bit value (0 or 1)
 * @param bit_index The position of the bit in the message
 *
 * @return The calculated frequency in Hertz
 */
uint32_t Modulate_GetFhbfskFrequency(bool bit, uint16_t bit_index);

/**
 * @brief Registers modulation parameters with the parameter system for HMI access
 *
 * Registers output amplitude with appropriate min/max constraints to make
 * it available for configuration through the system's HMI.
 *
 * @return true if all parameters registered successfully, false otherwise
 */
bool Modulate_RegisterParams();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_MODULATE_H_ */
