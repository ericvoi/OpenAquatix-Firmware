/*
 * check_inputs.h
 *
 *  Created on: Feb 17, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_CHECK_INPUTS_H_
#define COMMON_UTILS_CHECK_INPUTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Validates and converts a string to a uint8_t within specified range
 *
 * Validates if the input string represents a valid uint8_t number within
 * the specified range. Handles empty strings by converting to 0 and checking
 * against the min value.
 *
 * @param num_str The string to validate and convert
 * @param num_str_len Length of the string
 * @param ret Pointer to store the converted value if valid
 * @param min Minimum acceptable value (inclusive)
 * @param max Maximum acceptable value (inclusive)
 *
 * @return true if valid and within range, false otherwise
 *
 * @note Handles potential overflow by using larger intermediate type
 */
bool checkUint8(char* num_str, uint16_t num_str_len, uint8_t* ret, uint8_t min, uint8_t max);

/**
 * @brief Validates and converts a string to a uint16_t within specified range
 *
 * Validates if the input string represents a valid uint16_t number within
 * the specified range. Handles empty strings by converting to 0 and checking
 * against the min value.
 *
 * @param num_str The string to validate and convert
 * @param num_str_len Length of the string
 * @param ret Pointer to store the converted value if valid
 * @param min Minimum acceptable value (inclusive)
 * @param max Maximum acceptable value (inclusive)
 *
 * @return true if valid and within range, false otherwise
 *
 * @note Handles potential overflow by using larger intermediate type
 */
bool checkUint16(char* num_str, uint16_t num_str_len, uint16_t* ret, uint16_t min, uint16_t max);

/**
 * @brief Validates and converts a string to a uint32_t within specified range
 *
 * Validates if the input string represents a valid uint32_t number within
 * the specified range. Handles empty strings by converting to 0 and checking
 * against the min value.
 *
 * @param num_str The string to validate and convert
 * @param num_str_len Length of the string
 * @param ret Pointer to store the converted value if valid
 * @param min Minimum acceptable value (inclusive)
 * @param max Maximum acceptable value (inclusive)
 *
 * @return true if valid and within range, false otherwise
 *
 * @note Limited to values that fit within atoi's return range
 */
bool checkUint32(char* num_str, uint16_t num_str_len, uint32_t* ret, uint32_t min, uint32_t max);

/**
 * @brief Validates and converts a string to a float within specified range
 *
 * Validates if the input string represents a valid float number within
 * the specified range with no trailing characters.
 *
 * @param num_str The string to validate and convert
 * @param ret Pointer to store the converted value if valid
 * @param min Minimum acceptable value (inclusive)
 * @param max Maximum acceptable value (inclusive)
 *
 * @return true if valid and within range, false otherwise
 */
bool checkFloat(char* num_str, float* ret, float min, float max);

/**
 * @brief Validates a yes/no input character and converts to boolean
 *
 * Accepts 'y', 'Y', 'n', or 'N' as valid inputs and converts to
 * corresponding boolean value.
 *
 * @param input The character to validate ('y', 'Y', 'n', or 'N')
 * @param ret Pointer to store the converted boolean (true for y/Y, false for n/N)
 *
 * @return true if input was valid, false otherwise
 */
bool checkYesNo(char input, bool* ret);


/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_CHECK_INPUTS_H_ */
