/*
 * mess_dac_resources.h
 *
 *  Created on: Apr 29, 2025
 *      Author: ericv
 */

#ifndef COMMON_MESS_DAC_RESOURCES_H_
#define COMMON_MESS_DAC_RESOURCES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include "mess_dsp_config.h"
#include "dac_waveform.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes mutex for shared DAC and MESS resources
 */
void MessDacResource_Init(void);

/**
 * @brief Stores the configuration and bit message
 *
 * @param new_cfg Configuration structure with DSP parameters
 * @param new_bit_msg Bit message that is to be sent out
 *
 * @note pointers are not checked
 */
void MessDacResource_RegisterMessageConfiguration(const DspConfig_t* new_cfg,
    BitMessage_t* new_bit_msg);

/**
 * @brief Get the next waveform step
 *
 * @param current_step Current step in the bit message
 *
 * @return structure with the frequency, duration, and amplitude to transmit
 */
WaveformStep_t MessDacResource_GetStep(uint16_t current_step);

/**
 * @brief Number of steps in the synchronization sequence
 * 
 * Useful when no cfg is available
 * 
 * @return uint16_t Number of steps
 */
uint16_t MessDacResource_SyncSteps(void);

/**
 * @brief NUmber of steps needed to send wakeup tones
 * 
 * @return uint16_t Number of wakeup steps
 */
uint16_t MessDacResource_WakeupSteps(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_MESS_DAC_RESOURCES_H_ */
