/*
 * mess_adc.h
 *
 *  Created on: Feb 13, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_ADC_H_
#define MESS_MESS_ADC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define ADC_BUFFER_SIZE           1024 
#define PROCESSING_BUFFER_SIZE    (1 << 14) // 16384
#define PROCESSING_BUFFER_MASK    (PROCESSING_BUFFER_SIZE - 1)

#define ADC_SAMPLING_RATE         120000  // 120 kHz

/* Exported macro ------------------------------------------------------------*/

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern TIM_HandleTypeDef htim8;

extern volatile uint16_t input_head_pos;
extern volatile uint16_t input_tail_pos;
extern uint16_t input_buffer[PROCESSING_BUFFER_SIZE];

extern volatile uint16_t feedback_head_pos;
extern volatile uint16_t feedback_tail_pos;
extern uint16_t feedback_buffer[PROCESSING_BUFFER_SIZE];


/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the ADC subsystem
 *
 * Configures and starts Timer 8 which is used for ADC triggering,
 * resets buffer indices, and clears the ADC buffer.
 *
 * @return true if initialization succeeds, false otherwise
 */
bool ADC_Init();

/**
 * @brief Starts ADC conversions on the input channel using DMA
 *
 * Resets the input buffer index, starts Timer 8, and initiates DMA-based
 * ADC conversions for the input channel.
 *
 * @return true if operation starts successfully, false otherwise
 */
bool ADC_StartInput();

/**
 * @brief Starts ADC conversions on the feedback channel using DMA
 *
 * Resets the feedback buffer index and initiates DMA-based ADC conversions
 * for the feedback channel. Requires a feedback buffer to be registered first.
 *
 * @return true if operation starts successfully, false if feedback buffer is NULL or DMA fails
 *
 * @pre Feedback buffer must be registered with ADC_RegisterFeedbackBuffer()
 */
bool ADC_StartFeedback();

/**
 * @brief Stops ADC conversions on the feedback channel
 *
 * @return true if operation stops successfully, false otherwise
 */
bool ADC_StopFeedback();

/**
 * @brief Stops ADC conversions on the input channel
 *
 * @return true if operation stops successfully, false otherwise
 */
bool ADC_StopInput();

/**
 * @brief Stops all ADC conversions and resets buffer indices
 *
 * Stops both feedback and input ADC channels and resets their respective
 * buffer indices to zero.
 *
 * @return true if all operations stop successfully, false if any stop operation fails
 */
bool ADC_StopAll();

/**
 * @brief Resets input buffer head and tail to 0, and sets buffer to 0
 * 
 */
void ADC_InputClear();

/**
 * @brief Resets feedback buffer head and tail to 0, and sets buffer to 0
 * 
 */
void ADC_FeedbackClear();

/* Private defines -----------------------------------------------------------*/

// Inline functions to interface with the input ADC buffer
// Note: always_inline used to force basic optimization in low optimization
// levels to get the required performance in debug modes

static inline __attribute__((always_inline)) uint16_t ADC_InputAvailableSamples(void) 
{
  return (input_head_pos - input_tail_pos) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) void ADC_InputTailAdvance(uint16_t num_samples) 
{
  input_tail_pos = (input_tail_pos + num_samples) & PROCESSING_BUFFER_MASK;
}

static inline __attribute((always_inline)) void ADC_InputSetTail(uint16_t tail_location)
{
  input_tail_pos = tail_location;
}

static inline __attribute__((always_inline)) uint16_t ADC_InputGetData(uint16_t offset) 
{
  return input_buffer[(input_tail_pos + offset) & PROCESSING_BUFFER_MASK];
}

static inline __attribute__((always_inline)) uint16_t ADC_InputGetDataAbsolute(uint16_t position)
{
  return input_buffer[position];
}

static inline __attribute__((always_inline)) uint16_t ADC_InputGetTail(void)
{
  return input_tail_pos;
}

static inline __attribute__((always_inline)) uint16_t ADC_InputGetHead(void)
{
  return input_head_pos;
}

// Inline functions to interface with the feedback ADC buffer

static inline __attribute__((always_inline)) uint16_t ADC_FeedbackAvailableSamples(void) 
{
  return (feedback_head_pos - feedback_tail_pos) & PROCESSING_BUFFER_MASK;
}

static inline __attribute__((always_inline)) void ADC_FeedbackTailAdvance(uint16_t num_samples) 
{
  feedback_tail_pos = (feedback_tail_pos + num_samples) & PROCESSING_BUFFER_MASK;
}

static inline __attribute((always_inline)) void ADC_FeedbackSetTail(uint16_t tail_location)
{
  feedback_tail_pos = tail_location;
}

static inline __attribute__((always_inline)) uint16_t ADC_FeedbackGetData(uint16_t offset) 
{
  return feedback_buffer[(feedback_tail_pos + offset) & PROCESSING_BUFFER_MASK];
}

static inline __attribute__((always_inline)) uint16_t ADC_FeedbackGetDataAbsolute(uint16_t position)
{
  return feedback_buffer[position];
}

static inline __attribute__((always_inline)) uint16_t ADC_FeedbackGetTail(void)
{
  return feedback_tail_pos;
}

static inline __attribute__((always_inline)) uint16_t ADC_FeedbackGetHead(void)
{
  return feedback_head_pos;
}


#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_ADC_H_ */
