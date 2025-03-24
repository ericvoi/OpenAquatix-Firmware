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
#define PROCESSING_BUFFER_SIZE    16384 // must be a power of 2

#define ADC_SAMPLING_RATE         120000  // 120 kHz

/* Exported macro ------------------------------------------------------------*/

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc3;
extern TIM_HandleTypeDef htim8;


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
 * @brief Registers an external buffer for storing input ADC conversions
 *
 * @param in_buffer Pointer to the buffer where input ADC values will be stored
 *
 * @return true if registration succeeds, false if a buffer is already registered
 *
 * @note Can only be called once successfully until the module is reset
 */
bool ADC_RegisterInputBuffer(uint16_t* in_buffer);

/**
 * @brief Registers an external buffer for storing feedback ADC conversions
 *
 * @param fb_buffer Pointer to the buffer where feedback ADC values will be stored
 *
 * @return true if registration succeeds, false if a buffer is already registered
 *
 * @note Can only be called once successfully until the module is reset
 */
bool ADC_RegisterFeedbackBuffer(uint16_t* fb_buffer);

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

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_ADC_H_ */
