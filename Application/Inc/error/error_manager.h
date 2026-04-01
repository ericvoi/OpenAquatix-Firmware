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
  SubSystemId_t subsystem; 
} ErrorLutEntry_t;

/**
 * @brief List of errors that can occur, severity, description, and associated subsystem
 * 
 * @note the error description must be less than ERROR_DESCRIPTION_WIDTH characters
 */
#define ERROR_MANAGEMENT_TABLE(X)                                                                                                       \
                                                                                                                                        \
  /* --- RTOS Infrastructure --- */                                                                                                     \
  X(ERROR_QUEUE_RUNNING,                  ERROR_SEVERITY_WARN,            "Error adding message to queue",            SUBSYS_NONE)      \
  X(ERROR_FLAGS_RUNNING,                  ERROR_SEVERITY_ABORT,           "Error while reading flags",                SUBSYS_NONE)      \
  X(ERROR_MUTEX_TIMEOUT,                  ERROR_SEVERITY_ABORT,           "Timed out while waiting for mutex",        SUBSYS_NONE)      \
  X(ERROR_FLAGS_INITIALIZATION,           ERROR_SEVERITY_UNRECOVERABLE,   "Error initializing flags",                 SUBSYS_NONE)      \
  X(ERROR_QUEUE_INITIALIZATION,           ERROR_SEVERITY_UNRECOVERABLE,   "Error initializing queue",                 SUBSYS_NONE)      \
  X(ERROR_MUTEX_INITIALIZATION,           ERROR_SEVERITY_UNRECOVERABLE,   "Failed to initialize mutex",               SUBSYS_NONE)      \
  X(ERROR_TIMER_INITIALIZATION,           ERROR_SEVERITY_UNRECOVERABLE,   "Error timer initialization",               SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- Hardware Peripherals & AFE --- */                                                                                              \
  X(ERROR_FEEDBACK_ADC_INITIALIZATION,    ERROR_SEVERITY_ABORT,           "Error starting feedback ADC",              SUBSYS_NONE)      \
  X(ERROR_STARTING_TRANSDUCER_OUTPUT,     ERROR_SEVERITY_ABORT,           "Error starting transducer output",         SUBSYS_NONE)      \
  X(ERROR_TRANSDUCER_FB_INITIALIZATION,   ERROR_SEVERITY_ABORT,           "Error starting transducer feedback",       SUBSYS_NONE)      \
  X(ERROR_INPUT_ADC_INITIALIZATION,       ERROR_SEVERITY_TASK_RESET,      "Error starting input ADC",                 SUBSYS_NONE)      \
  X(ERROR_STOPPING_TRANSDUCER_OUTPUT,     ERROR_SEVERITY_TASK_RESET,      "Error stopping transducer output",         SUBSYS_NONE)      \
  X(ERROR_PGA_COMMAND,                    ERROR_SEVERITY_DISABLE_SUBSYS,  "Error sending PGA command",                SUBSYS_PGA)       \
  X(ERROR_AFE_TIMEOUT,                    ERROR_SEVERITY_FULL_RESET,      "AFE power rail transition timed out",      SUBSYS_NONE)      \
  X(ERROR_AFE_GENERAL,                    ERROR_SEVERITY_FULL_RESET,      "Misc. AFE error",                          SUBSYS_NONE)      \
  X(ERROR_WAVEFORM_STEP,                  ERROR_SEVERITY_UNRECOVERABLE,   "Error in waveform step generation",        SUBSYS_NONE)      \
  X(ERROR_DAC_FLUSH,                      ERROR_SEVERITY_UNRECOVERABLE,   "Error flushing DAC",                       SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- Monitoring --- */                                                                                                              \
  X(ERROR_RGB_LED,                        ERROR_SEVERITY_WARN,            "Error updating RGB LED colour",            SUBSYS_NONE)      \
  X(ERROR_INA_READ,                       ERROR_SEVERITY_RESET_SUBSYS,    "Failed read of INA219 registers",          SUBSYS_INA)       \
  X(ERROR_POWER_MONITOR,                  ERROR_SEVERITY_DISABLE_SUBSYS,  "Error in power monitor",                   SUBSYS_INA)       \
  X(ERROR_PRESSURE_SENSOR,                ERROR_SEVERITY_DISABLE_SUBSYS,  "Error in pressure sensor",                 SUBSYS_LPS)       \
  X(ERROR_JUNCTION_TEMPERATURE,           ERROR_SEVERITY_DISABLE_SUBSYS,  "Error in obtaining junction temp",         SUBSYS_TJ)        \
                                                                                                                                        \
  /* --- Signal Processing --- */                                                                                                       \
  X(ERROR_INTERLEAVING_DEPTH,             ERROR_SEVERITY_WARN,            "No valid interleaving depth",              SUBSYS_NONE)      \
  X(ERROR_ADC_BUFFER_OVERFLOW,            ERROR_SEVERITY_TASK_RESET,      "A signal processing buffer",               SUBSYS_NONE)      \
  X(ERROR_OVERFLOW_SYNC,                  ERROR_SEVERITY_TASK_RESET,      "Overflow in synchronization buffers",      SUBSYS_NONE)      \
  X(ERROR_ANALYSIS_BUFFER_OVERFLOW,       ERROR_SEVERITY_TASK_RESET,      "Analysis buffer overflowed",               SUBSYS_NONE)      \
  X(ERROR_VITERBI_TRACEBACK,              ERROR_SEVERITY_TASK_RESET,      "Viterbi traceback error",                  SUBSYS_NONE)      \
  X(ERROR_FILT_EVENTS,                    ERROR_SEVERITY_TASK_RESET,      "Multiple unexpected FILT events",          SUBSYS_NONE)      \
  X(ERROR_AGC,                            ERROR_SEVERITY_DISABLE_SUBSYS,  "Error in AGC subsystem",                   SUBSYS_PGA)       \
  X(ERROR_FFT_INITIALIZATION,             ERROR_SEVERITY_UNRECOVERABLE,   "Error initializing FFT",                   SUBSYS_NONE)      \
  X(ERROR_INVALID_NOISE_BIN,              ERROR_SEVERITY_UNRECOVERABLE,   "Invalid noise bin range",                  SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- Acoustic Protocol & MAC --- */                                                                                                 \
  X(ERROR_UNKNOWN_MESSAGE,                ERROR_SEVERITY_WARN,            "Invalid message type received",            SUBSYS_NONE)      \
  X(ERROR_RANGING_REQUEST_OVERWRITTEN,    ERROR_SEVERITY_WARN,            "Overwrote existing ranging request",       SUBSYS_NONE)      \
  X(ERROR_INVALID_CARGO_LENGTH,           ERROR_SEVERITY_ABORT,           "Invalid cargo length in message",          SUBSYS_NONE)      \
  X(ERROR_INVALID_PREAMBLE_FIELD,         ERROR_SEVERITY_ABORT,           "Invalid preamble field in message",        SUBSYS_NONE)      \
  X(ERROR_EXCEED_BIT_MSG_LEN,             ERROR_SEVERITY_ABORT,           "Attempted write outside bit array",        SUBSYS_NONE)      \
  X(ERROR_INVALID_RESERVATION_TIME,       ERROR_SEVERITY_ABORT,           "Invalid reservation time",                 SUBSYS_NONE)      \
  X(ERROR_UNKNOWN_JANUS,                  ERROR_SEVERITY_ABORT,           "Received unsupported JANUS message",       SUBSYS_NONE)      \
  X(ERROR_INVALID_CHARACTER,              ERROR_SEVERITY_ABORT,           "Message contained an invalid character",   SUBSYS_NONE)      \
  X(ERROR_SEND_UNKNOWN_JANUS,             ERROR_SEVERITY_ABORT,           "Attempted JANUS message not supported",    SUBSYS_NONE)      \
  X(ERROR_CSMA_BEB_TIMEOUT,               ERROR_SEVERITY_TASK_RESET,      "CSMA with BEB background noise timeout",   SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- Configuration & Parameters --- */                                                                                              \
  X(ERROR_FAILED_PARAM_NUM_ERASES,        ERROR_SEVERITY_WARN,            "Failed to update flash erase count",       SUBSYS_NONE)      \
  X(ERROR_PARAMETER_REGISTRATION,         ERROR_SEVERITY_UNRECOVERABLE,   "Error registering parameters",             SUBSYS_NONE)      \
  X(ERROR_PARAMETER_ACCESS,               ERROR_SEVERITY_UNRECOVERABLE,   "Cannot access parameter",                  SUBSYS_NONE)      \
  X(ERROR_INVALID_PARAMETER_VERSION,      ERROR_SEVERITY_UNRECOVERABLE,   "Invalid parameter version number",         SUBSYS_NONE)      \
  X(ERROR_SETTING_PARAMETER,              ERROR_SEVERITY_UNRECOVERABLE,   "Failed to set parameter",                  SUBSYS_NONE)      \
  X(ERROR_PARAMETER_FLASH_WRITE,          ERROR_SEVERITY_UNRECOVERABLE,   "Failed to write a parameter to flash",     SUBSYS_NONE)      \
  X(ERROR_FAILED_PARAM_FLASH_RESET,       ERROR_SEVERITY_UNRECOVERABLE,   "Failed to reset parameter flash",          SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- HMI & COMM --- */                                                                                                              \
  X(ERROR_PRINT_WAVEFORM_OVERFLOW,        ERROR_SEVERITY_ABORT,           "Overflow in print waveform buffer",        SUBSYS_NONE)      \
  X(ERROR_MENU_REGISTRATION,              ERROR_SEVERITY_UNRECOVERABLE,   "Failed to register menu",                  SUBSYS_NONE)      \
  X(ERROR_USB_HMI,                        ERROR_SEVERITY_WARN,            "Failed to write data to HMI",              SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- General Software Errors --- */                                                                                                 \
  X(ERROR_ERROR_MANAGEMENT_INTERNAL,      ERROR_SEVERITY_WARN,            "Internal error management error",          SUBSYS_NONE)      \
  X(ERROR_GENERAL_WARN_ISR,               ERROR_SEVERITY_WARN,            "Unexpected error in an ISR",               SUBSYS_NONE)      \
  X(ERROR_NULL_PTR,                       ERROR_SEVERITY_ABORT,           "NULL pointer",                             SUBSYS_NONE)      \
  X(ERROR_EXCEED_TEMP_BUFFER_LEN,         ERROR_SEVERITY_ABORT,           "Attempted write outside buffer bounds",    SUBSYS_NONE)      \
  X(ERROR_SANITY_CHECK,                   ERROR_SEVERITY_ABORT,           "Failed sanity check",                      SUBSYS_NONE)      \
  X(ERROR_UNHANDLED_CASE,                 ERROR_SEVERITY_UNRECOVERABLE,   "Unexpected case in switch statement",      SUBSYS_NONE)      \
  X(ERROR_INVALID_FUNCTION_PARAMETERS,    ERROR_SEVERITY_UNRECOVERABLE,   "Invalid function parameters (not NULL)",   SUBSYS_NONE)      \
                                                                                                                                        \
  /* --- Feedback Tests --- */                                                                                                          \
  X(ERROR_FBK_TEST_COMPARE,               ERROR_SEVERITY_WARN,            "Error creating reference message",         SUBSYS_NONE)      \
  X(ERROR_FEEDBACK_TEST_INITIALIZATION,   ERROR_SEVERITY_DISABLE_SUBSYS,  "Error initializing feedback tests",        SUBSYS_FBK_TESTS) \
  X(ERROR_FBK_TEST_INDEX,                 ERROR_SEVERITY_DISABLE_SUBSYS,  "Invalid feedback test index",              SUBSYS_FBK_TESTS)

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

typedef enum {
  OA_STATUS_ERROR,  // Includes task resets and subsystems being disabled
  OA_STATUS_ABORT,  // Includes subsystem resets as well
  OA_STATUS_WARN,
  OA_STATUS_OK
} OpenAquatixStatus_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/

#define RETURN_IF_ERROR_PRESENT(func) do { \
  func; \
  ErrorCheck_t ret_status = Error_CheckStatus(); \
  if (ret_status == ERROR_QUIT) return; \
} while (0)

#define REGISTER_ERROR(error_code) \
  RETURN_IF_ERROR_PRESENT(Error_RegisterError(error_code, __FILE_NAME__, __LINE__)) 

// Special error handlers to use when the function returns an integer not void
#define RETURN_IF_ERROR_PRESENT_NON_VOID(func, ret_val) do { \
  func; \
  ErrorCheck_t ret_status = Error_CheckStatus(); \
  if (ret_status == ERROR_QUIT) return ret_val; \
} while (0)

#define REGISTER_ERROR_NON_VOID(error_code, ret_val) \
  RETURN_IF_ERROR_PRESENT_NON_VOID(Error_RegisterError(error_code, __FILE_NAME__, __LINE__), ret_val)

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
 * @brief Current system status
 * 
 * @note Status automatically times out errors/warnings if they were first
 * raised sufficiently long ago
 * 
 * @return OpenAquatixStatus_t Current status
 */
OpenAquatixStatus_t Error_GetStatus(void);

/**
 * @brief Call this when a warm reset is detected
 * 
 * If a warm reset has occurred, the last error timestamp is set to the current
 * timestamp
 */
void Error_LogWarmReset(void);

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
