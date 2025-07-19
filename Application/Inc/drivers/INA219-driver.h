/*
 * INA219-driver.h
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

#ifndef __INA219_DRIVER_H_
#define __INA219_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/

#define INA219_ADDRESS          0x40    // Device base address
#define POWER_ADDRESS           0x00    // Power register address


/* Exported functions prototypes ---------------------------------------------*/

/** 
 *@brief Initializes the INA219 Power monitor
 *
 * Configures the INA219 with default settings and prepares it for operation.
 *
 * @return HAL_StatusTypeDef HAL status (HAL_OK if successful)
 *
 * @note This function should be called once during system initialization
 *       before any other INA219 functions are used.
 */
HAL_StatusTypeDef INA219_Init();


/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __INA219_DRIVER_H_ */
