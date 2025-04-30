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

void MessDacResource_Init(void);
void MessDacResource_RegisterMessageConfiguration(const DspConfig_t* new_cfg,
    BitMessage_t* new_bit_msg);
WaveformStep_t MessDacResource_GetStep(uint16_t current_step);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_MESS_DAC_RESOURCES_H_ */
