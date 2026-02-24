/*
 * error_log.h
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef ERROR_ERROR_LOG_H_
#define ERROR_ERROR_LOG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "error_manager.h"
#include "comm_main.h"
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  const char* task_name;
  const char* file_name;
  uint16_t line_number;
  uint16_t occurrences;
  uint16_t reset_count;
  OpenAquatixErrors_t error_code;
  uint64_t timestamp;
} ErrorEntry_t;

/* Exported constants --------------------------------------------------------*/

#define MAX_ENTRIES_IN_ERROR_LOG                40

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Logs an error in the error log
 * 
 * @param file_name File where the error originally occurred
 * @param task_name Task that was running the code that caused the error
 * @param line Line in the file where the error occurred
 * @param error_code Code corresponding to error that occurred
 * 
 * @warning This function should only be called by the error manager
 */
void ErrorLog_LogError(const char* file_name, const char* task_name, 
                       uint16_t line, OpenAquatixErrors_t error_code);

/**
 * @brief Prints the error log to the HMI
 * 
 * @note This function should only be called by the COMM task 
 */
void ErrorLog_PrintLog(CommInterface_t interface);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_LOG_H_ */
