/*
 * error_subsys.h
 *
 *  Created on: Mar 6, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef ERROR_ERROR_SUBSYS_H_
#define ERROR_ERROR_SUBSYS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  SUBSYS_DISABLE,
  SUBSYS_RESET,
  SUBSYS_PROCEED
} SubSystemStatus_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void ErrorSubsys_Disable(SubSystemId_t subsys);

void ErrorSubsys_RequestReset(SubSystemId_t subsys);

SubSystemStatus_t ErrorSubsys_CurrentStatus(SubSystemId_t subsys);

void ErrorSubsys_ClearReset(SubSystemId_t subsys);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_SUBSYS_H_ */
