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
#include "mess_dsp_config.h"
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

float Modulate_GetAmplitude(uint32_t freq_hz);

/**
 * @brief Initializes and starts the transducer output subsystem
 *
 * Performs a controlled sequence of shutting down active peripherals,
 * starting ADC feedback, enabling DAC waveform output, and starting
 * the timer. Includes necessary delays for peripheral stabilization.
 *
 * @return true if all peripherals started successfully, false otherwise
 */
bool Modulate_StartTransducerOutput(uint16_t num_steps, 
                                    const DspConfig_t* new_cfg, 
                                    BitMessage_t* new_bit_msg);

/**
 * @brief Implements all processes needed to start modulating via feedback
 * 
 * Stops all used peripherals, registers the message, and then starts
 * peripherals again in a controlled sequence
 * 
 * @param num_steps The number of steps in the bit message sequence
 * @param new_cfg The configuration to use for the next modulation
 * @param new_bit_msg The bit message to send out through the feedback network
 * 
 * @return true if all peripherals successfully started, false otherwise
 */
bool Modulate_StartFeedbackOutput(uint16_t num_steps, 
                                  const DspConfig_t* new_cfg, 
                                  BitMessage_t* new_bit_msg);

/**
 * @brief Generates a simple two-frequency test sequence for transducer testing
 *
 * Creates a test pattern alternating between 30kHz and 33kHz with 1ms durations.
 * Uses the currently configured output amplitude.
 */
void Modulate_TestOutput();

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
 * @param cfg Configuration information
 *
 * @return The calculated frequency in Hertz
 */
uint32_t Modulate_GetFhbfskFrequency(bool bit, 
                                     uint16_t bit_index, 
                                     const DspConfig_t* cfg);

/**
 * @brief Calculates the frequency for a given bit using FSK modulation
 * 
 * Implements Frequency-Shift Keying by returing the frequency corresponding to
 * a bit
 * 
 * @param bit The bit value (1 or 0)
 * @param cfg Configuration information
 * 
 * @return The calculated frequency in Hertz
 */
uint32_t Modulate_GetFskFrequency(bool bit, const DspConfig_t* cfg);

/**
 * @brief Populates the waveform step for data symbols
 * 
 * @param cfg DSP configuration for the bit's transmission
 * @param bit_msg Bit message to take the bits from
 * @param waveform_step Waveform step to populate with symbol information
 * @param transmission_step Step in transmission, corresponds to bit index
 * @return true if successful, false otherwise
 */
bool Modulate_DataStep(const DspConfig_t* cfg, BitMessage_t* bit_msg, WaveformStep_t* waveform_step, uint16_t bit_index, uint16_t symbol_index);

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
