/*
 * error_reset.h
 *
 *  Created on: Feb 25, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef ERROR_ERROR_RESET_H_
#define ERROR_ERROR_RESET_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Resets the device without clearing the log by setting a flag
 */
void ErrorReset_WarmReset(void);

/**
 * @brief Notifies error manager that the reset condition was error-induced
 */
void ErrorReset_NotifyErrorReset(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_RESET_H_ */
