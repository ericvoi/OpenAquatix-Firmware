/*
 * comm_function_loops.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

#ifndef COMM_COMM_FUNCTION_LOOPS_H_
#define COMM_COMM_FUNCTION_LOOPS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "comm_menu_system.h"
#include "cfg_parameters.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

extern char uninitialized_parameter_message[];
extern char error_limits_message[];
extern char error_updating_message[];

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Processes a uint32_t parameter update through an interactive state machine
 *
 * Displays current parameter value, prompts for a new value, validates against
 * parameter limits, and updates if valid. Handles error cases including uninitialized
 * parameters and values outside permitted ranges.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to modify
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 */
void COMMLoops_LoopUint32(FunctionContext_t* context, ParamIds_t param_id);

/**
 * @brief Processes a uint16_t parameter update through an interactive state machine
 *
 * Displays current parameter value, prompts for a new value, validates against
 * parameter limits, and updates if valid. Handles error cases including uninitialized
 * parameters and values outside permitted ranges.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to modify
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 */
void COMMLoops_LoopUint16(FunctionContext_t* context, ParamIds_t param_id);

/**
 * @brief Processes a uint8_t parameter update through an interactive state machine
 *
 * Displays current parameter value, prompts for a new value, validates against
 * parameter limits, and updates if valid. Handles error cases including uninitialized
 * parameters and values outside permitted ranges.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to modify
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 */
void COMMLoops_LoopUint8(FunctionContext_t* context, ParamIds_t param_id);

/**
 * @brief Processes a float parameter update through an interactive state machine
 *
 * Displays current parameter value, prompts for a new value, validates against
 * parameter limits, and updates if valid. Handles error cases including uninitialized
 * parameters and values outside permitted ranges.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to modify
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 */
void COMMLoops_LoopFloat(FunctionContext_t* context, ParamIds_t param_id);

/**
 * @brief Processes an enumeration parameter update with descriptive text options
 *
 * Displays current parameter value with its text description, shows all available
 * options with descriptions, and processes user selection. Validates input and
 * handles error conditions.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to modify
 * @param descriptors Array of string descriptions for each enumeration value
 * @param num_descriptors Number of descriptions in the array
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 * @warning The num_descriptors must equal max+1 (parameter range size)
 */
void COMMLoops_LoopEnum(FunctionContext_t* context, ParamIds_t param_id, char** descriptors, uint16_t num_descriptors);

/**
 * @brief Processes a boolean parameter toggle through an interactive state machine
 *
 * Displays current boolean state (enabled/disabled) and allows the user to
 * toggle it to the opposite state with confirmation. Parameter is stored
 * internally as a uint8_t.
 *
 * @param context Communication context containing state machine and I/O buffers
 * @param param_id Identifier of the parameter to toggle
 *
 * @note Must be called repeatedly until state becomes PARAM_STATE_COMPLETE
 */
void COMMLoops_LoopToggle(FunctionContext_t* context, ParamIds_t param_id);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMM_COMM_FUNCTION_LOOPS_H_ */
