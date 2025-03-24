/*
 * dau_card-driver.h
 *
 *  Created on: Mar 17, 2025
 *      Author: ericv
 */

#ifndef DRIVERS_DAU_CARD_DRIVER_H_
#define DRIVERS_DAU_CARD_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "comm_main.h"
#include <stdbool.h>


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the Data Acquisition Unit (DAU) communication interface
 *
 * Configures the DAU buffer parameters, enables UART IDLE line detection
 * interrupt, and initiates DMA-based UART reception.
 *
 * @note This function must be called before any other DAU functions
 */
void DAU_Init(void);

/**
 * @brief Transmits data over the DAU UART interface using DMA
 *
 * Acquires a mutex to ensure thread-safe access to the UART, waits if there's
 * an ongoing transmission, copies data to the transmit buffer, and initiates
 * DMA transfer.
 *
 * @param data Pointer to the data to be transmitted
 * @param len Length of data in bytes
 *
 * @note Thread-safe implementation using RTOS mutex
 * @warning Function silently returns if data length exceeds buffer size
 */
void DAU_TransmitData(uint8_t* data, uint16_t len);

/**
 * @brief Processes newly received data from the DMA circular buffer
 *
 * Calculates the current DMA position, updates the circular buffer pointers,
 * and processes all newly received bytes between the head and tail pointers.
 *
 * @note Should be called regularly to process incoming data
 */
void DAU_GetNewData(void);

/**
 * @brief Processes received data bytes and handles special characters
 *
 * Parses incoming data, handling special characters:
 * - Escape ('\e'): Immediately marks data as ready with escape code
 * - Backspace ('\b'): Removes last character if possible
 * - CR/LF ('\r' or '\n'): Marks current buffer as ready if not empty
 * - Other characters: Added to buffer if space available
 *
 * @param data Pointer to received data bytes
 * @param len Number of bytes to process
 *
 * @note Buffer overflow protection is implemented
 */
void DAU_ProcessRxData(uint8_t* data, uint32_t len);

/**
 * @brief Retrieves processed message from the DAU buffer
 *
 * Checks if new content is available, copies data to the provided buffer,
 * and updates buffer state flags. The buffer is reset if a complete message was read.
 *
 * @param buffer Destination buffer to copy message into
 * @param len Pointer to variable that will receive the message length
 *
 * @return RxState_t Status indicating:
 *         - DATA_READY: A complete message was available and copied
 *         - NEW_CONTENT: Partial content available and copied
 *         - NO_CHANGE: No new content available
 */
RxState_t DAU_GetMessage(uint8_t* buffer, uint16_t* len);


/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_DAU_CARD_DRIVER_H_ */
