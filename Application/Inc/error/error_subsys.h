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

/**
 * @brief Disables a subsystem by ID
 * 
 * @param subsys The subsystem to disable
 */
void ErrorSubsys_Disable(SubSystemId_t subsys);

/**
 * @brief Sets flag that a subsystem should reset
 * 
 * @param subsys The subsystem to reset
 */
void ErrorSubsys_RequestReset(SubSystemId_t subsys);

/**
 * @brief Get the current status of a subsystem
 * 
 * @param subsys Subsystem to get the status of
 * @return SubSystemStatus_t SUBSYS_DISABLE Abort any and all operations in subsystem
 *                           SUBSYS_RESET Reset the subsystem
 *                           SUBSYS_PROCEED Continue normal operation of subsystem
 */
SubSystemStatus_t ErrorSubsys_CurrentStatus(SubSystemId_t subsys);

/**
 * @brief Clears reset condition in the subsystem
 * 
 * @param subsys The subsystem to clear the reset condition of
 */
void ErrorSubsys_ClearReset(SubSystemId_t subsys);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_SUBSYS_H_ */
