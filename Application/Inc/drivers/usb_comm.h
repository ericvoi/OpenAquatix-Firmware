/*
 * usb_comm.h
 *
 *  Created on: Feb 1, 2025
 *      Author: ericv
 */

#ifndef __USB_COMM_H_
#define __USB_COMM_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "usbd_cdc_if.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include "comm_main.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/

extern osSemaphoreId_t usbSemaphoreHandle;

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the USB communication interface
 *
 * Sets up the USB buffer structure with default values and prepares
 * the interface for receiving data.
 */
void USB_Init(void);

/**
 * @brief Transmits data over the USB interface
 *
 * Sends data over USB with thread synchronization using a semaphore
 * to ensure exclusive access to the USB hardware.
 *
 * @param data Pointer to the data buffer to transmit
 * @param len Number of bytes to transmit
 *
 * @note This function blocks indefinitely until the semaphore is acquired
 * @note Includes a 1ms delay after transmission for hardware stability
 */
void USB_TransmitData(uint8_t* data, uint16_t len);

/**
 * @brief Processes received USB data
 *
 * Handles incoming USB data by processing special characters and buffering
 * regular characters until a complete message is formed.
 *
 * @param data Pointer to the received data buffer
 * @param len Number of bytes received
 *
 * @note Special character handling:
 *       - ESC ('\e'): Immediately marks data as ready with ESC as the only character
 *       - Backspace ('\b'): Removes the last character if buffer is not empty
 *       - CR/LF ('\r' or '\n'): Terminates current message and marks as ready
 * @note Silently discards data if buffer overflow occurs
 */
void USB_ProcessRxData(uint8_t* data, uint32_t len);

/**
 * @brief Retrieves a message from the USB buffer
 *
 * Checks if new data is available and copies it to the provided buffer
 * if there have been changes since the last call.
 *
 * @param buffer Destination buffer where the message will be copied
 * @param len Pointer to variable that will receive the message length
 *
 * @return RxState_t indicating the state of the received data:
 *         - NO_CHANGE: No new data received
 *         - NEW_CONTENT: New data received but message is incomplete
 *         - DATA_READY: Complete message received
 *
 * @note The buffer is cleared after copying if a complete message was received
 */
RxState_t USB_GetMessage(uint8_t* buffer, uint16_t* len);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __USB_COMM_H_ */
