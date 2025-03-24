/*
 * comm_main.h
 *
 *  Created on: Feb 2, 2025
 *      Author: ericv
 */

#ifndef __COMM_MAIN_H_
#define __COMM_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/


/* Exported types ------------------------------------------------------------*/

#define MAX_COMM_IN_BUFFER_SIZE   256
#define MAX_COMM_OUT_BUFFER_SIZE  512

#define CALC_LEN                  0 // A length of 0 makes the function call strlen

typedef enum {
  NEW_CONTENT,
  DATA_READY,
  NO_CHANGE
} RxState_t;

typedef enum {
  COMM_USB,
  COMM_UART,
  COMM_BOTH
} CommInterface_t;

typedef struct {
  uint8_t buffer[MAX_COMM_IN_BUFFER_SIZE];
  uint16_t length;
  uint16_t index;
  bool contents_changed;
  bool data_ready;
  CommInterface_t source;
} CommBuffer_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Entry point for the communication interface task
 *
 * Initializes communication subsystems (USB and DAU), registers with the parameter system,
 * and manages the HMI. This task handles menu navigation, command processing, and message
 * routing between different communication interfaces.
 *
 * @param argument Task parameter (unused, required by RTOS task signature)
 *
 * @note This function never returns and should be started as an RTOS task
 * @warning Calls Error_Routine() if any initialization step fails
 */
void COMM_StartTask(void *argument);

/**
 * @brief Transmits data through the specified communication interface(s)
 *
 * Sends data through USB, UART, or both interfaces based on the interface parameter.
 *
 * @param data Pointer to the data buffer to transmit
 * @param data_len Length of data in bytes (if 0, strlen is used to calculate length)
 * @param interface Target communication interface (COMM_USB, COMM_UART, or COMM_BOTH)
 */
void COMM_TransmitData(const void *data, uint32_t data_len, CommInterface_t interface);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __COMM_MAIN_H_ */
