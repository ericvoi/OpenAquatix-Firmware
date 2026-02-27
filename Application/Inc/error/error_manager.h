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
  X(ERROR_SEVERITY_DISABLE_SUBSYS, "Disable subsytem") \
  X(ERROR_SEVERITY_FULL_RESET, "Full reset") \
  X(ERROR_SEVERITY_UNRECOVERABLE, "Fatal loop")

DECLARE_ENUM(ERROR_SEVERITY_TABLE, NUM_ERROR_SEVERITY, ErrorSeverity_t)

typedef struct {
  ErrorSeverity_t severity;
  const char* description;
  SubSystemIds_t subsystem; 
} ErrorLutEntry_t;

/**
 * @brief List of errors that can occur, severity, description, and associated subsystem
 * 
 * @note the error description must be less than ERROR_DESCRIPTION_WIDTH characters
 */
#define ERROR_MANAGEMENT_TABLE(X) \
  X(ERROR_ERROR_MANAGEMENT_INTERNAL, ERROR_SEVERITY_WARN, "Internal error management error", SUBSYS_NONE) \
  X(ERROR_ADC_BUFFER_OVERFLOW, ERROR_SEVERITY_TASK_RESET, "A signal processing buffer", SUBSYS_NONE) \
  X(ERROR_PGA_COMMAND, ERROR_SEVERITY_DISABLE_SUBSYS, "Error sending PGA command", SUBSYS_PGA) \
  X(ERROR_PARAMETER_REGISTRATION, ERROR_SEVERITY_UNRECOVERABLE, "Error registering parameters", SUBSYS_NONE) \
  X(ERROR_FFT_INITIALIZATION, ERROR_SEVERITY_UNRECOVERABLE, "Error initializing FFT", SUBSYS_NONE) \
  X(ERROR_FEEDBACK_TEST_INITIALIZATION, ERROR_SEVERITY_DISABLE_SUBSYS, "Error initializing feedback tests", SUBSYS_FBK_TESTS) \
  X(ERROR_UNHANDLED_CASE, ERROR_SEVERITY_UNRECOVERABLE, "Unexpected case in switch statement", SUBSYS_NONE) \
  X(ERROR_FLAGS_INITIALIZATION, ERROR_SEVERITY_UNRECOVERABLE, "Error initializing flags", SUBSYS_NONE) \
  X(ERROR_QUEUE_INITIALIZATION, ERROR_SEVERITY_UNRECOVERABLE, "Error initializing queue", SUBSYS_NONE) \
  X(ERROR_INPUT_ADC_INITIALIZATION, ERROR_SEVERITY_TASK_RESET, "Error starting input ADC", SUBSYS_NONE) \
  X(ERROR_FEEDBACK_ADC_INITIALIZATION, ERROR_SEVERITY_ABORT, "Error starting feedback ADC", SUBSYS_NONE) \
  X(ERROR_AGC, ERROR_SEVERITY_DISABLE_SUBSYS, "Error in AGC subsystem", SUBSYS_PGA) \
  X(ERROR_FLAGS_RUNNING, ERROR_SEVERITY_ABORT, "Error while reading flags", SUBSYS_NONE) \
  X(ERROR_NULL_PTR, ERROR_SEVERITY_ABORT, "NULL pointer", SUBSYS_NONE) \
  X(ERROR_PARAMETER_ACCESS, ERROR_SEVERITY_UNRECOVERABLE, "Cannot access parameter", SUBSYS_NONE) \
  X(ERROR_INVALID_CARGO_LENGTH, ERROR_SEVERITY_ABORT, "Invalid cargo length in message", SUBSYS_NONE) \
  X(ERROR_INVALID_PREAMBLE_FIELD, ERROR_SEVERITY_ABORT, "Invalid preamble field in message", SUBSYS_NONE) \
  X(ERROR_EXCEED_BIT_MSG_LEN, ERROR_SEVERITY_ABORT, "Attempted write outside bit array", SUBSYS_NONE) \
  X(ERROR_INVALID_RESERVATION_TIME, ERROR_SEVERITY_ABORT, "Invalid reservation time", SUBSYS_NONE) \
  X(ERROR_UNKNOWN_MESSAGE, ERROR_SEVERITY_WARN, "Invalid message type received", SUBSYS_NONE) \
  X(ERROR_UNKNOWN_JANUS, ERROR_SEVERITY_BLOCK, "Received unsupported JANUS message", SUBSYS_NONE) 

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

#define RETURN_IF_ERROR_PRESENT(func) do { \
  func; \
  ErrorCheck_t ret_status = Error_CheckStatus(); \
  if (ret == ERROR_QUIT) return; \
} while (0);

#define REGISTER_ERROR(error_code) \
  RETURN_IF_ERROR_PRESENT(Error_RegisterError(error_code, __FILE_NAME__, __LINE__)) \

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Registers a task and sets its internal name
 * 
 * @param task_name String with the task name
 * 
 * @note This must be called for every task before the system will initialize
 */
void Error_RegisterTask(const char* task_name);

/**
 * @brief Called by each task when all of their parameters have been registered
 * 
 * Signals that the task is ready to proceed. Once this is done, parameters are
 * loaded from flash and the application can proceed
 */
void Error_ParameterRegistrationComplete(void);

/**
 * @brief Registers an error, logs it, and takes appropriate action
 * 
 * @param error_code Error that has occurred
 * @param file File name where error occurred (__FILE_NAME__)
 * @param line Line number where error occurred (__LINE__)
 */
void Error_RegisterError(OpenAquatixErrors_t error_code, const char* file, uint16_t line);

/**
 * @brief Describes an error code
 * 
 * @param error_code Error code to describe
 * @return const char* String containing error description (null-terminated)
 */
const char* Error_GetDescription(OpenAquatixErrors_t error_code);

/**
 * @brief Describes severity of an error code
 * 
 * @param error_code Error code to get the severity of
 * @return const char* String containing severity description (null-terminated)
 */
const char* Error_GetSeverity(OpenAquatixErrors_t error_code);

/**
 * @brief Checks if program flow should continue
 * 
 * @return ErrorCheck_t ERROR_PROCEED if ok to continue
 *                      ERROR_QUIT    if blocked or pending reset
 * 
 * @note This must be called when any upstream function can cause an error
 */
ErrorCheck_t Error_CheckStatus(void);

/**
 * @brief Resets the abort flag for a task
 * 
 * @note This should only be called at the end of every task loop
 */
void Error_ResetAbortFlag(void);

/**
 * @brief Checks if a module needs to reset itself
 * 
 * @return TaskResetStatus_t TASK_PROCEED if ok to continue
 *                           TASK_RESET if task reset is required
 */
TaskResetStatus_t Error_CheckModuleReset(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* ERROR_ERROR_MANAGER_H_ */
