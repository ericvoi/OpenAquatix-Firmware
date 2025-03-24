/*
 * sine_lut.h
 *
 *  Created on: Feb 5, 2025
 *      Author: ericv
 */

#ifndef __DAC_WAVEFORM_H_
#define __DAC_WAVEFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  uint32_t freq_hz;
  float relative_amplitude;
  uint32_t duration_us;
  uint32_t phase_increment;
} WaveformStep_t;

/* Exported constants --------------------------------------------------------*/

#define DAC_BUFFER_SIZE     500
#define DAC_SAMPLE_RATE     1000000

/* Exported macro ------------------------------------------------------------*/

extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim6;  // Timer for sequence timing


/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the DAC waveform generator system
 *
 * Sets up the sine lookup table, initializes the waveform control structure,
 * and configures the timer needed for DAC operation.
 *
 * @return true if initialization was successful, false if timer initialization failed
 *
 * @note Must be called before any other DAC waveform functions
 */
bool DAC_InitWaveformGenerator(void);

/**
 * @brief Configures a sequence of waveform steps for generation
 *
 * Sets up a sequence of waveform steps (frequency, duration, etc.) for the DAC
 * to output. Validates that all step durations are compatible with the DAC buffer
 * configuration and calculates necessary phase increments.
 *
 * @param sequence Pointer to an array of waveform steps
 * @param num_steps Number of steps in the sequence
 *
 * @return true if sequence is valid and was set successfully, false otherwise
 *
 * @note Each step's duration must be a multiple of half the DAC buffer duration
 *       (currently DAC_BUFFER_SIZE/2 * DAC_SAMPLE_RATE/1000000 microseconds)
 */
bool DAC_SetWaveformSequence(WaveformStep_t* sequence, uint32_t num_steps);

/**
 * @brief Starts the waveform output on the specified DAC channel
 *
 * Initiates DMA-based waveform generation on the specified DAC channel using
 * the previously configured waveform sequence.
 *
 * @param channel DAC channel to use (DAC_CHANNEL_1 or DAC_CHANNEL_2)
 *
 * @return true if output started successfully, false if no sequence is set or DMA failed
 *
 * @pre DAC_SetWaveformSequence must be called successfully before this function
 */
bool DAC_StartWaveformOutput(uint32_t channel);

/**
 * @brief Stops all DAC waveform output
 *
 * Terminates DMA transfers to both DAC channels and stops any related ADC feedback.
 *
 * @return true (operation always succeeds)
 */
bool DAC_StopWaveformOutput(void);

/**
 * @brief Checks if the DAC waveform generator is currently running
 *
 * @return true if DAC is actively generating waveforms, false otherwise
 */
bool DAC_IsRunning(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __DAC_WAVEFORM_H_ */
