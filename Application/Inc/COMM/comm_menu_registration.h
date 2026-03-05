/*
 * comm_menu_registration.h
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef __COMM_MENU_REGISTRATION_H_
#define __COMM_MENU_REGISTRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void COMM_RegisterMainMenu(void);
void COMM_RegisterConfigurationMenu(void);
void COMM_RegisterDebugMenu(void);
void COMM_RegisterHistoryMenu(void);
void COMM_RegisterTxRxMenu(void);
void COMM_RegisterEvalMenu(void);
void COMM_RegisterJanusMenu(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __COMM_MENU_REGISTRATION_H_ */
