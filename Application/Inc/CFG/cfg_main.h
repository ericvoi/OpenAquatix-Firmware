/*
 * cfg_main.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

#ifndef CFG_CFG_MAIN_H_
#define CFG_CFG_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define EVENT_ALL_TASKS_REGISTERED  0x01
#define EVENT_PARAMS_LOADED         0x02
#define EVENT_SAVE_REQUESTED        0x04

/* Exported macro ------------------------------------------------------------*/

extern osEventFlagsId_t param_events;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Configuration task entry point for parameter initialization
 *
 * This function serves as the entry point for the configuration task in the RTOS system.
 * It performs a sequential initialization process:
 * 1. Registers itself as "CFG" task
 * 2. Registers its parameters
 * 3. Waits for all tasks to complete their parameter registration
 * 4. Loads parameters from non-volatile flash storage
 * 5. Signals to other tasks that parameters are loaded and available
 * 6. Enters an infinite loop maintaining the task's existence
 *
 * @param argument Task argument pointer (unused but required by RTOS task signature)
 *
 * @note This task coordinates the system-wide parameter initialization sequence and
 *       must complete before other tasks can use their configuration parameters
 */
void CFG_StartTask(void* argument);
/**
 * @brief Creates the global parameter event flags object
 *
 * Initializes a new RTOS event flags object for parameter-related
 * event signaling. The object is created with default attributes
 * and stored in the global 'param_events' variable.
 *
 * @return true if the event flags were successfully created,
 *         false if creation failed
 *
 * @note This function should be called once during system initialization
 *       before any parameter operations are performed
 */
bool CFG_CreateParamFlags(void);
/**
 * @brief Blocks until parameter loading is complete
 *
 * Suspends the calling thread until the EVENT_PARAMS_LOADED flag is set in the
 * parameter event group. This function will wait indefinitely and does not
 * clear the event flag upon return.
 *
 * @warning This function blocks indefinitely and should not be called from
 * interrupt context or critical timing paths
 */
void CFG_WaitLoadComplete(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* CFG_CFG_MAIN_H_ */
