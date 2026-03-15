/*
 * mess_filt_resources.h
 *
 *  Created on: Feb 24, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMMON_MESS_FILT_RESOURCES_H_
#define COMMON_MESS_FILT_RESOURCES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define INPUT_ADC                 hadc2
#define FEEDBACK_ADC              hadc1

#define ADC_BUFFER_SIZE           1024 
#define PROCESSING_BUFFER_POWER   (14)
#define PROCESSING_BUFFER_SIZE    (1 << PROCESSING_BUFFER_POWER) // 16384
#define PROCESSING_BUFFER_MASK    (PROCESSING_BUFFER_SIZE - 1)

#define ADC_BITS                  (16U)

#define ADC_SAMPLING_RATE         120000  // 120 kHz
#define ADC_VREF                  (3.3f)

/* Exported macro ------------------------------------------------------------*/

extern volatile uint16_t input_head_pos;
extern volatile uint16_t input_processing_tail_pos;
extern volatile uint16_t input_noise_tail_pos;
extern float* input_buffer;

extern volatile uint16_t feedback_head_pos;
extern volatile uint16_t feedback_tail_pos;
extern uint16_t* feedback_buffer;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the ADC subsystem
 *
 * Configures and starts Timer 8 which is used for ADC triggering,
 * resets buffer indices, and clears the ADC buffer.
 */
void MessFiltResources_Init();

/**
 * @brief Starts ADC conversions on the input channel using DMA
 *
 * Resets the input buffer index, starts Timer 8, and initiates DMA-based
 * ADC conversions for the input channel.
 */
void MessFiltResources_StartInputAdc();

/**
 * @brief Starts ADC conversions on the feedback channel using DMA
 *
 * Resets the feedback buffer index and initiates DMA-based ADC conversions
 * for the feedback channel. Requires a feedback buffer to be registered first.
 *
 * @pre Feedback buffer must be registered with MessFiltResources_RegisterFeedbackBuffer()
 */
void MessFiltResources_StartFeedbackAdc();

/**
 * @brief Stops ADC conversions on the feedback channel
 *
 * @return true if operation stops successfully, false otherwise
 */
bool MessFiltResources_StopFeedbackAdc();

/**
 * @brief Stops ADC conversions on the input channel
 *
 * @return true if operation stops successfully, false otherwise
 */
bool MessFiltResources_StopInputAdc();

/**
 * @brief Stops all ADC conversions and resets buffer indices
 *
 * Stops both feedback and input ADC channels and resets their respective
 * buffer indices to zero.
 *
 * @return true if all operations stop successfully, false if any stop operation fails
 */
bool MessFiltResources_StopAllAdcs();

/**
 * @brief Resets input buffer head and tail to 0, and sets buffer to 0
 * 
 */
void MessFiltResources_InputAdcClear();

/**
 * @brief Resets feedback buffer head and tail to 0, and sets buffer to 0
 * 
 */
void MessFiltResources_FeedbackAdcClear();

/**
 * @brief Adds a number of filtered floats to the shared buffer
 * 
 * @param buf Buffer containing the samples to add
 * @param num_samples The number of samples to add from the buffer
 * @param cyccnt Cycle count of the last sample added to the array
 */
void MessFiltResources_AddFilteredSamples(float* buf, uint16_t num_samples, uint32_t cyccnt);

/**
 * @brief Gets the associated cyccnt for a given position and rollover count
 * 
 * @param position ADC position in index
 * @param rollovers Number of ADC rollovers
 * @return uint32_t CYCCNT for the position and rollover
 */
uint32_t MessFiltResources_AssociatedCyccnt(uint16_t position, uint32_t rollovers);

/**
 * @brief Number of times the ADC head has reset (both buffers)
 * 
 * @return uint32_t Number of rollovers for the tail
 */
uint32_t MessFiltResources_HeadRolloverCount();

/**
 * @brief Number of buffer rollovers at the buffer tail position
 * 
 * @param feedback Whether the feedback or the input ADC is being used
 * @return uint32_t Number of rollovers for the tail
 */
uint32_t MessFiltResources_TailRolloverCount(bool feedback);

// Inline functions to interface with the input ADC buffer
// Note: always_inline used to force basic optimization in low optimization
// levels to get the required performance in debug modes

static inline __attribute__((always_inline)) uint16_t MessFiltResources_AvailableProcessingSamples(void) 
{
  return (input_head_pos - input_processing_tail_pos) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_AvailableNoiseSamples(void) 
{
  return (input_head_pos - input_noise_tail_pos) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) void MessFiltResources_ProcessingTailAdvance(uint16_t num_samples) 
{
  input_processing_tail_pos = (input_processing_tail_pos + num_samples) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) void MessFiltResources_NoiseTailAdvance(uint16_t num_samples) 
{
  input_noise_tail_pos = (input_noise_tail_pos + num_samples) & PROCESSING_BUFFER_MASK;
}

static inline __attribute((always_inline)) void MessFiltResources_SetProcessingTail(uint16_t tail_location)
{
  input_processing_tail_pos = tail_location;
}

static inline __attribute((always_inline)) void MessFiltResources_SetNoiseTail(uint16_t tail_location)
{
  input_noise_tail_pos = tail_location;
}

static inline __attribute__((always_inline)) float MessFiltResources_GetProcessingData(uint16_t offset) 
{
  return input_buffer[(input_processing_tail_pos + offset) & PROCESSING_BUFFER_MASK];
}

static inline __attribute__((always_inline)) float MessFiltResources_GetNoiseData(uint16_t offset) 
{
  return input_buffer[(input_noise_tail_pos + offset) & PROCESSING_BUFFER_MASK];
}

static inline __attribute__((always_inline)) float MessFiltResources_GetInputDataAbsolute(uint16_t position)
{
  return input_buffer[position];
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_GetProcessingTail(void)
{
  return input_processing_tail_pos;
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_GetNoiseTail(void)
{
  return input_noise_tail_pos;
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_GetInputAdcHead(void)
{
  return input_head_pos;
}

// Inline functions to interface with the feedback ADC buffer

static inline __attribute__((always_inline)) uint16_t MessFiltResources_FeedbackAvailableSamples(void) 
{
  return (feedback_head_pos - feedback_tail_pos) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) void MessFiltResources_FeedbackTailAdvance(uint16_t num_samples) 
{
  feedback_tail_pos = (feedback_tail_pos + num_samples) & PROCESSING_BUFFER_MASK;
}

static inline __attribute((always_inline)) void MessFiltResources_FeedbackSetTail(uint16_t tail_location)
{
  feedback_tail_pos = tail_location;
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_FeedbackGetData(uint16_t offset) 
{
  return feedback_buffer[(feedback_tail_pos + offset) & PROCESSING_BUFFER_MASK];
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_FeedbackGetDataAbsolute(uint16_t position)
{
  return feedback_buffer[position];
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_FeedbackGetTail(void)
{
  return feedback_tail_pos;
}

static inline __attribute__((always_inline)) uint16_t MessFiltResources_FeedbackGetHead(void)
{
  return feedback_head_pos;
}

#ifdef __cplusplus
}
#endif

#endif /* COMMON_MESS_FILT_RESOURCES_H_ */
