/*
 * wakeup_tones.h
 *
 *  Created on: Aug 2, 2025
 *      Author: ericv
 */

#ifndef SLEEP_WAKEUP_TONES_H_
#define SLEEP_WAKEUP_TONES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_dsp_config.h"
#include "dac_waveform.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

// Transmission

/**
 * @brief Populates a waveform step with wakeup tones
 * 
 * @param cfg Signal processing parameters defining wakeup tones
 * @param waveform_step Waveform step to transmit (modified)
 * @param step_index Step of the transmit sequence
 * @return true if valid step index, false otherwise
 */
bool WakeupTones_GetStep(const DspConfig_t* cfg, WaveformStep_t* waveform_step, uint16_t step_index);

/**
 * @brief Number of steps in the transmission wakeup sequence
 * 
 * @param cfg Signal processing parameters defining wakeup tones
 * @return Number of steps required to transmit wakeup tones
 */
uint16_t WakeupTones_NumSteps(const DspConfig_t* cfg);

// Reception

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_WAKEUP_TONES_H_ */
