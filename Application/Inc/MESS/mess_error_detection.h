/*
 * mess_error_correction.h
 *
 *  Created on: Feb 12, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_ERROR_DETECTION_H_
#define MESS_MESS_ERROR_DETECTION_H_

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "mess_packet.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  CRC_8,
  CRC_16,
  CRC_32,
  CHECKSUM_8,
  CHECKSUM_16,
  CHECKSUM_32,
  NO_ERROR_DETECTION,
  NUM_ERROR_DETECTION_METHODS
} ErrorDetectionMethod_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Adds error correction data to a bit message
 *
 * Calculates and appends CRC or checksum data to the bit message based on the
 * currently selected error correction method. Updates the final_length field
 * of the message accordingly.
 *
 * @param bit_msg Pointer to the bit message to modify
 *
 * @return true if correction data was successfully added,
 *         false if calculation failed or correction method is invalid
 *
 * @see ErrorDetection_CheckDetection
 */
bool ErrorDetection_AddDetection(BitMessage_t* bit_msg);

/**
 * @brief Verifies error correction data in a bit message
 *
 * Checks the integrity of a received bit message using the currently
 * selected error correction method (CRC or checksum).
 *
 * @param bit_msg Pointer to the bit message to check
 * @param error Output parameter set to true if an error is detected
 *
 * @return true if verification was performed successfully,
 *         false if verification failed or correction method is invalid
 */
bool ErrorDetection_CheckDetection(BitMessage_t* bit_msg, bool* error);

/**
 * @brief Gets the bit length of the current error correction method
 *
 * @param length Output parameter to receive the bit length of the
 *               current error correction method
 *
 * @return true if length was set successfully,
 *         false if the current correction method is invalid
 */
bool ErrorDetection_CheckLength(uint16_t* length);

/**
 * @brief Registers error correction parameters with the system
 *
 * Registers the error correction method parameter for HMI access.
 *
 * @return true if registration was successful, false otherwise
 */
bool ErrorDetection_RegisterParams(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_ERROR_DETECTION_H_ */
