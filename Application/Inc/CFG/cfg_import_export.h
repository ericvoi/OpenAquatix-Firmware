/*
 * cfg_import_export.h
 *
 *  Created on: Apr 17, 2025
 *      Author: ericv
 * 
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef CFG_CFG_IMPORT_EXPORT_H_
#define CFG_CFG_IMPORT_EXPORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "comm_menu_system.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Exports select configuration options in text format to HMI
 *
 * Exports predetermined list of configuration values to the HMI by first
 * converting the data to human readable text. Values exported by this function
 * are only identifiable by their parameter id so it is made for peer-peer rather
 * than debugging or an overall view
 *
 * @param context Gives a buffer and interface context. Used to return state
 *
 * @return true if successful, false otherwise
 *
 * @see ImportExport_ImportSomeParameters
 */
bool ImportExport_ExportSomeParameters(FunctionContext_t* context);

/**
 * @brief Exports all configuration options in text format to HMI
 *
 * Exports predetermined list of configuration values to the HMI by first
 * converting the data to human readable text. Values exported by this function
 * are only identifiable by their parameter id so it is made for peer-peer rather
 * than debugging or an overall view
 * 
 * @param context Gives a buffer and interface context. Used to return state
 * 
 * @return true if successfully exported all parameters (no issues with configuration)
 */
bool ImportExport_ExportAllParameters(FunctionContext_t* context);

/**
 * @brief Imports configuration values exported from another device
 *
 * 2 stages: The first stage prompts the user to input a configuration string.
 * The second stage parses the input and updates the input parameters.
 *
 * @param context Gives an output buffer, the string to parse, the string to
 *        parse's length, and the interface context. Changes the context's state
 * 
 * @return true if all values within bounds and parameters referenced are all registered
 *
 * @note The number of parameters imported and exported must be the same.
 * @see ImportExport_ExportSomeParameters
 */
bool ImportExport_ImportSomeParameters(FunctionContext_t* context);

/**
 * @brief Imports all configuration values exported from another device
 * 
 * 2 stages: The first stage prompts the user to input a configuration string.
 * The second stage parses the input and updates the input parameters.
 * 
 * @param context Gives an output buffer, the string to parse, the string to
 *        parse's length, and the interface context. Changes the context's state
 * @return true if parameters imported 
 */
bool ImportExport_ImportAllParameters(FunctionContext_t* context);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CFG_CFG_IMPORT_EXPORT_H_ */
