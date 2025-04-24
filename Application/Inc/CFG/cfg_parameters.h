/*
 * cfg_parameters.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

#ifndef CFG_CFG_PARAMETERS_H_
#define CFG_CFG_PARAMETERS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/

#include "main.h"
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/

typedef enum {
  PARAM_BAUD,
  PARAM_OUTPUT_AMPLITUDE,
  PARAM_MSG_START_FCN,
  PARAM_FSK_F0,
  PARAM_FSK_F1,
  PARAM_MOD_DEMOD_METHOD,
  PARAM_FC,
  PARAM_FHBFSK_FREQ_SPACING,
  PARAM_FHBFSK_DWELL_TIME,
  PARAM_PRINT_ENABLED,
  PARAM_FHBFSK_NUM_TONES,
  PARAM_EVAL_MODE_ON,
  PARAM_EVAL_MESSAGE,
  PARAM_ID,
  PARAM_STATIONARY_FLAG,
  PARAM_ERROR_CORRECTION,
  PARAM_DEMODULATION_DECISION,
  PARAM_DAC_TRANSITION_LEN,
  PARAM_MODULATION_OUTPUT_METHOD,
  PARAM_MODULATION_TARGET_POWER,
  PARAM_R,
  PARAM_C0,
  PARAM_L0,
  PARAM_C1,
  // The lower frequency must be lower than the upper frequency but this is not
  // enforced when setting the parameter only when a calibration is performed
  PARAM_MOD_CAL_LOWER_FREQ,
  PARAM_MOD_CAL_UPPER_FREQ,

  PARAM_MAX_TRANSDUCER_VOLTAGE,
  // The lower frequency must be lower than the upper frequency but this is not
  // enforced when setting the parameter only when a calibration is performed
  PARAM_DEMOD_CAL_LOWER_FREQ,
  PARAM_DEMOD_CAL_UPPER_FREQ,

  PARAM_HISTORICAL_COMPARISON_THRESHOLD,
  PARAM_LED_BRIGHTNESS,
  PARAM_LED_ENABLE,
  PARAM_AGC_ENABLE,
  PARAM_FIXED_PGA_GAIN,
  // Add new parameters here and nowhere else
  NUM_PARAM
} ParamIds_t;

typedef enum {
  PARAM_TYPE_UINT8,
  PARAM_TYPE_INT8,
  PARAM_TYPE_UINT16,
  PARAM_TYPE_INT16,
  PARAM_TYPE_UINT32,
  PARAM_TYPE_INT32,
  PARAM_TYPE_FLOAT
} ParamType_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the parameter management subsystem
 *
 * Sets up the parameter management system by creating a recursive mutex for
 * thread-safe parameter access and initializing the task registration array.
 * This function must be called before any other parameter-related functions.
 *
 * @return true if initialization was successful, false if mutex creation failed
 *         or if NUM_PARAM exceeds MAX_PARAMETERS
 *
 * @note Uses a recursive mutex to allow nested parameter operations from the same task
 * @warning Must be called once during system initialization before any tasks use parameters
 */
bool Param_Init(void);

/**
 * @brief Loads saved configuration parameters from flash
 *
 * Loops through the entire flash sector for saved parameters and saves them
 * one at a time to RAM. Also resets the is_modified flag
 *
 * @return true if successful and false if the version of an entry is incorrect
 * or updating a parameter failed
 *
 * @note Must be called after all parameters have been registered
 */
bool Param_LoadInit(void);

/**
 * @brief Registers a parameter in the HMI parameter system
 *
 * Registers a parameter with the specified attributes and stores it in the global
 * parameter array indexed by the parameter ID. The function validates the input,
 * acquires a mutex for thread safety, and sets up parameter limits based on type.
 *
 * @param id Unique identifier for the parameter
 * @param name Display name for the parameter (will be truncated if exceeds buffer size)
 * @param type Data type of the parameter (PARAM_TYPE_UINT8, PARAM_TYPE_INT32, etc.)
 * @param value_ptr Pointer to the actual parameter value in memory
 * @param value_size Size of the parameter value in bytes
 * @param min Pointer to minimum allowed value (must match parameter type)
 * @param max Pointer to maximum allowed value (must match parameter type)
 *
 * @return true if registration successful, false if failed (NULL value_ptr,
 *         already registered parameter, mutex acquisition failure, or invalid type)
 *
 * @note This function only stores references to the provided memory locations and does not allocate memory
 * @warning The value_ptr, min, and max pointers must remain valid for the lifetime of the parameter
 */
bool Param_Register(ParamIds_t id, const char* name, ParamType_t type,
                    void* value_ptr, size_t value_size, void* min, void* max);

/**
 * @brief Retrieves a parameter value by ID
 *
 * Core function that retrieves a parameter value from the parameter store.
 * This function provides thread-safe access to parameters by using mutex protection.
 *
 * @param id The parameter identifier
 * @param value Pointer to memory where the parameter value will be stored
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *         (parameter not initialized or mutex acquisition failed)
 *
 * @note This function blocks indefinitely while waiting for mutex acquisition
 * @warning The caller must ensure that value points to sufficient memory to
 *          store the parameter's full size
 */
bool Param_GetValue (ParamIds_t id, void* value);

/**
 * @brief Retrieves an 8-bit unsigned parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved uint8_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetUint8 (ParamIds_t id, uint8_t* value);

/**
 * @brief Retrieves an 8-bit signed parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved int8_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetInt8  (ParamIds_t id, int8_t* value);

/**
 * @brief Retrieves a 16-bit unsigned parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved uint16_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetUint16(ParamIds_t id, uint16_t* value);

/**
 * @brief Retrieves a 16-bit signed parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved int16_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetInt16 (ParamIds_t id, int16_t* value);

/**
 * @brief Retrieves a 32-bit unsigned parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved uint32_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetUint32(ParamIds_t id, uint32_t* value);

/**
 * @brief Retrieves a 32-bit signed parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved int32_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetInt32 (ParamIds_t id, int32_t* value);

/**
 * @brief Retrieves a floating point parameter value
 *
 * @param id The parameter identifier
 * @param value Pointer to store the retrieved int32_t value
 *
 * @return true if parameter was successfully retrieved, false otherwise
 *
 * @see Param_GetValue
 */
bool Param_GetFloat (ParamIds_t id, float* value);

/**
 * @brief Retrieves the name of a parameter by its ID
 *
 * Looks up a parameter in the parameter database and returns its name.
 * The function is thread-safe through mutex protection of the parameter database.
 *
 * @param id The parameter identifier to look up
 *
 * @return Pointer to the parameter name if parameter exists and is initialized,
 *         NULL if parameter is not initialized or mutex cannot be acquired
 *
 * @note This function blocks indefinitely while waiting for the mutex
 * @see findParamById, isParamInitialized
 */
char* Param_GetName (ParamIds_t id);

/**
 * @brief Retrieves the minimum and maximum limits for a specified parameter
 *
 * This is the base function that all type-specific limit retrieval functions use.
 * Acquires a parameter mutex to ensure thread safety while accessing the parameter.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value (must accommodate sizeof(uint32_t) bytes)
 * @param max Pointer to receive the maximum limit value (must accommodate sizeof(uint32_t) bytes)
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note This function always copies sizeof(uint32_t) bytes regardless of the actual parameter type
 * @note Blocks indefinitely until the parameter mutex can be acquired
 */
bool Param_GetLimits(ParamIds_t id, void* min, void* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as uint8_t
 *
 * Converts the internal 32-bit representation to uint8_t values.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as uint8_t
 * @param max Pointer to receive the maximum limit value as uint8_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note Values may be truncated when cast from uint32_t to uint8_t
 */
bool Param_GetUint8Limits (ParamIds_t id, uint8_t* min, uint8_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as int8_t
 *
 * Converts the internal 32-bit representation to int8_t values.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as int8_t
 * @param max Pointer to receive the maximum limit value as int8_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note Values may be truncated when cast from int32_t to int8_t
 */
bool Param_GetInt8Limits  (ParamIds_t id, int8_t* min, int8_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as uint16_t
 *
 * Converts the internal 32-bit representation to uint16_t values.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as uint16_t
 * @param max Pointer to receive the maximum limit value as uint16_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note Values may be truncated when cast from uint32_t to uint16_t
 */
bool Param_GetUint16Limits(ParamIds_t id, uint16_t* min, uint16_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as int16_t
 *
 * Converts the internal 32-bit representation to int16_t values.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as int16_t
 * @param max Pointer to receive the maximum limit value as int16_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note Values may be truncated when cast from int32_t to int16_t
 */
bool Param_GetInt16Limits (ParamIds_t id, int16_t* min, int16_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as uint32_t
 *
 * Direct passthrough to Param_GetLimits as the underlying storage is uint32_t.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as uint32_t
 * @param max Pointer to receive the maximum limit value as uint32_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 */
bool Param_GetUint32Limits(ParamIds_t id, uint32_t* min, uint32_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as int32_t
 *
 * Direct passthrough to Param_GetLimits interpreting the underlying data as int32_t.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as int32_t
 * @param max Pointer to receive the maximum limit value as int32_t
 *
 * @return true if limits were successfully retrieved, false otherwise
 */
bool Param_GetInt32Limits (ParamIds_t id, int32_t* min, int32_t* max);

/**
 * @brief Retrieves the minimum and maximum limits for a parameter as float
 *
 * Direct passthrough to Param_GetLimits interpreting the underlying data as float.
 *
 * @param id The parameter identifier
 * @param min Pointer to receive the minimum limit value as float
 * @param max Pointer to receive the maximum limit value as float
 *
 * @return true if limits were successfully retrieved, false otherwise
 *
 * @note Assumes the parameter's internal representation can be safely reinterpreted as float
 */
bool Param_GetFloatLimits (ParamIds_t id, float* min, float* max);

/**
 * @brief Sets a parameter value with type checking and range validation
 *
 * This function updates a parameter's value after validating it against the
 * parameter's defined limits. The operation is protected by a mutex to ensure
 * thread safety in the RTOS environment.
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the new value (type must match the parameter's type)
 *
 * @return true if parameter was successfully set, false if:
 *         - mutex could not be acquired
 *         - parameter was not found
 *         - parameter was not initialized
 *         - value was outside the parameter's limits
 *
 * @note Only updates the parameter if the new value differs from the current value
 * @note Marks the parameter as modified when changed, but does not immediately save to non-volatile storage
 * @warning The caller must ensure the value pointer points to data of the correct type
 */
bool Param_SetValue(ParamIds_t id, const void* value);

/**
 * @brief Sets an 8-bit unsigned integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the uint8_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetUint8(ParamIds_t id, uint8_t* value);

/**
 * @brief Sets an 8-bit signed integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the int8_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetInt8(ParamIds_t id, int8_t* value);

/**
 * @brief Sets a 16-bit unsigned integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the uint16_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetUint16(ParamIds_t id, uint16_t* value);

/**
 * @brief Sets a 16-bit signed integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the int16_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetInt16(ParamIds_t id, int16_t* value);

/**
 * @brief Sets a 32-bit unsigned integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the uint32_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetUint32(ParamIds_t id, uint32_t* value);

/**
 * @brief Sets a 32-bit signed integer parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the int32_t value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetInt32(ParamIds_t id, int32_t* value);

/**
 * @brief Sets a floating point parameter
 *
 * Type-specific wrapper for Param_SetValue().
 *
 * @param id Identifier of the parameter to set
 * @param value Pointer to the float value
 *
 * @return true if parameter was successfully set, false otherwise
 *
 * @see Param_SetValue() for details on validation and error conditions
 */
bool Param_SetFloat(ParamIds_t id, float* value);

/**
 * @brief Returns the parameter type for a parameter
 *
 * @param id Identifier for the parameter
 * @param param_type Type for the parameter
 *
 * @return true if parameter exists, false otherwise
 *
 * @note Blocks indefinitely while waiting for mutex acquisition
 * @note The parameter must be set
 */
bool Param_GetParamType(ParamIds_t id, ParamType_t* param_type);

/**
 * @brief Saves parameters to flash (non-volatile) memory
 *
 * Loops through the parameters and checks if one has changed. If a parameter
 * has changed, save it to the next flash word. If the next address is outside
 * of the flash sector, reset flash. Can save multiple parameters
 *
 * @return true if all modified parameters saved
 *         false if error saving parameter
 */
bool Param_SaveToFlash(void);

/**
 * @brief Registers a task in the parameter management system
 *
 * Adds a task to the registry if space is available and the task ID is unique.
 * The task is initially marked as not fully registered.
 *
 * @param task_id Unique identifier for the task
 * @param task_name Name of the task (will be truncated if longer than TASK_NAME_LEN-1)
 *
 * @return true if registration successful, false if registry is full or task_id already exists
 *
 * @note Blocks indefinitely while waiting for mutex acquisition
 * @see Param_TaskRegistrationComplete
 */
bool Param_RegisterTask(TaskIds_t task_id, const char* task_name);

/**
 * @brief Completes the registration process for a task
 *
 * Marks a previously registered task as fully registered. When all tasks
 * (NUM_TASKS) complete registration, this function triggers the
 * EVENT_ALL_TASKS_REGISTERED event.
 *
 * @param task_id The task ID to mark as fully registered
 *
 * @return true if task was found and successfully marked as registered,
 *         false if task ID not found or already registered
 *
 * @note Blocks indefinitely while waiting for mutex acquisition
 * @see Param_RegisterTask
 */
bool Param_TaskRegistrationComplete(TaskIds_t task_id);

/**
 * @brief Resets the flash memory for parameters
 *
 * Resets the flash sector designated for parameters and then increments
 * the number of erases and writes it to the first block
 *
 * @return true if no errors,
 *         false if errors
 */
bool Param_FlashReset(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CFG_CFG_PARAMETERS_H_ */
