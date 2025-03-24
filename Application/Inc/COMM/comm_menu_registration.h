/*
 * comm_menu_registration.h
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
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

bool COMM_RegisterMainMenu(void);
bool COMM_RegisterConfigurationMenu(void);
bool COMM_RegisterDebugMenu(void);
bool COMM_RegisterHistoryMenu(void);
bool COMM_RegisterTxRxMenu(void);
bool COMM_RegisterEvalMenu(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __COMM_MENU_REGISTRATION_H_ */
