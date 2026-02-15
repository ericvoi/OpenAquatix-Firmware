/*
 * dac_switch.h
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef AFE_INTERNAL_DAC_SWITCH_H_
#define AFE_INTERNAL_DAC_SWITCH_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  DAC_DIRECTION_TRANSDUCER,
  DAC_DIRECTION_FEEDBACK
} DacDirection_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Changes the direction of the analog switch. Finished in < 30 ns
 * 
 * @param direction Whether DAC channel should point towards transducer or feedback
 */
void DACSwitch_Change(DacDirection_t direction);

/**
 * @brief Current direction of the DAC switch
 * 
 * @return DacDirection_t Enum defining whether DAC connected to feedback or power amplifier/transducer
 */
DacDirection_t DACSwitch_Current(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_INTERNAL_DAC_SWITCH_H_ */
