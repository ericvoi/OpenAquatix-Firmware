/*
 * error_manager.h
 *
 *  Created on: Feb 20, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef ERROR_ERROR_MANAGER_H_
#define ERROR_ERROR_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "main.h"
#include "cfg_parameters.h"
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/

#define ERROR_SEVERITY_TABLE(X) \
  X(ERROR_SEVERITY_WARN, "Warning") \
  X(ERROR_SEVERITY_ABORT, "Abort action") \
  X(ERROR_SEVERITY_TASK_RESET, "Reset task") \
  X(ERROR_SEVERITY_RESET_SUBSYS, "Reset subsytem") \
  X(ERROR_SEVERITY_FULL_RESET, "Full reset")

DECLARE_ENUM(ERROR_SEVERITY_TABLE, NUM_ERROR_SEVERITY, ErrorSeverity_t)

typedef struct {
  ErrorSeverity_t severity;
  const char* description;
  SubSystemIds_t subsystem; 
} ErrorLutEntry_t;

#define ERROR_MANAGEMENT_TABLE(X) \
  X(ERROR_ERROR_MANAGEMENT_INTERNAL, ERROR_SEVERITY_WARN, "Internal error management error", SUBSYS_NONE) \
  X(ERROR_ADC_BUFFER_OVERFLOW, ERROR_SEVERITY_TASK_RESET, "A signal processing buffer", SUBSYS_NONE) \

#define XERROR_ENUM(error, severity, description, subsystem) error,

typedef enum { ERROR_MANAGEMENT_TABLE(XERROR_ENUM) NUM_UNIQUE_ERRORS } OpenAquatixErrors_t;
#undef XERROR_ENUM

typedef enum {
  ERROR_PROCEED,
  ERROR_QUIT
} ErrorCheck_t;

typedef enum {
  TASK_PROCEED,
  TASK_RESET
} TaskResetStatus_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/

#define REGISTER_ERROR(error_code) \
  Error_RegisterError(error_code, __FILE_NAME__, __LINE__);

#define RETURN_IF_ERROR_PRESENT do { \
  ErrorCheck_t ret = Error_CheckStatus(); \
  if (ret == ERROR_QUIT) return; \
} while (0);

/* Exported functions prototypes ---------------------------------------------*/

void Error_RegisterTask(const char* task_name);
void Error_ParameterRegistrationComplete(void);
void Error_RegisterError(OpenAquatixErrors_t error_code, const char* file, uint16_t line);
const char* Error_GetDescription(OpenAquatixErrors_t error_code);
const char* Error_GetSeverity(OpenAquatixErrors_t error_code);
ErrorCheck_t Error_CheckStatus(void);
void Error_ResetAbortFlag(void);
TaskResetStatus_t Error_CheckModuleReset(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_MANAGER_H_ */
