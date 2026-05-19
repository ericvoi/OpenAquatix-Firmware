/*
 * sine_lut.h
 *
 *  Created on: Feb 5, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
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

typedef enum {
  OUTPUT_CONSTANT_SQUARE,     // Square envelope of a constant frequency and amplitude
  OUTPUT_CONSTANT_TUKEY,      // ^ With Tukey window applied
  OUTPUT_LFM_CHIRP            // Linear-frequency sweep from freq_hz to f_end_hz over duration_us
} ModulationOutputType_t;

typedef struct {
  ModulationOutputType_t output_type;
  uint32_t freq_hz;           // Start frequency (also the carrier for non-LFM steps)
  uint32_t f_end_hz;          // End frequency for OUTPUT_LFM_CHIRP; ignored otherwise
  float relative_amplitude;
  uint32_t duration_us;
} WaveformStep_t;

typedef enum {
  FILL_FIRST_HALF,
  FILL_LAST_HALF
} FillType_t;

/* Exported constants --------------------------------------------------------*/

#define DAC_BUFFER_SIZE     500
#define DAC_SUBSAMPLING     2
#define DAC_SAMPLE_RATE     (1000000 / DAC_SUBSAMPLING)

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
 * @note Must be called before any other DAC waveform functions
 */
void Waveform_InitWaveformGenerator(void);

/**
 * @brief Configures the maximum number of waveform steps for the output
 * 
 * This is used to generate blocks of the transmission output
 *
 * @param num_steps Number of steps in the sequence
 * @param is_message Whether the transmission is a full message or not
 * @param delay Whether to delay the transmission until a specific cycle
 * @param cyccnt The CYCCNT to start transmission at. Only relevant if delay is true
 *
 * @return true if sequence is valid and was set successfully, false otherwise
 */
bool Waveform_SetWaveformSequence(uint16_t num_steps, bool is_message, bool delay, uint32_t cyccnt);

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
 * @pre Waveform_SetWaveformSequence must be called successfully before this function
 */
bool Waveform_PrepareWaveformOutput(uint32_t channel);

/**
 * @brief Starts waveform output with optional delay if configured
 * 
 * @pre Waveform_SetWaveformSequence must be called successfully before this function
 * @pre Waveform_PrepareWaveformOutput must be called successfully before this function
 */
void Waveform_StartOutput(void);

/**
 * @brief Starts output for a ranging request and logs the CYCCNT it started
 * 
 * Analogous to Waveform_StartOutput for ranging requests
 * 
 * @pre Waveform_SetWaveformSequence must be called successfully before this function
 * @pre Waveform_PrepareWaveformOutput must be called successfully before this function
 */
void Waveform_SendRangingRequest(void);

/**
 * @brief Stops all DAC waveform output
 *
 * Terminates DMA transfers to both DAC channels and stops any related ADC feedback.
 *
 * @return true (operation always succeeds)
 */
bool Waveform_StopWaveformOutput(void);

/**
 * @brief Checks if the DAC waveform generator is currently running
 *
 * @return true if DAC is actively generating waveforms, false otherwise
 */
bool Waveform_IsRunning(void);

/**
 * @brief Registers module parameters with the parameter system
 *
 * @note Logs an error in the event of a failure
 */
void Waveform_RegisterParams(void);

/**
 * @brief Sends a flushing message through the dac
 *
 * @note This must be called before starting the DAC
 */
void Waveform_Flush(void);

/**
 * @brief Fills half of the DAC DMA buffer
 *
 * @param type Fill the first or last half of the buffer
 */
void Waveform_FillBuffer(FillType_t type);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __DAC_WAVEFORM_H_ */
