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

/**
 * @brief Callback function for I2C memory transfer completion
 *
 * This function is called when an I2C memory transfer is completed.
 * It sets the transfer_complete flag to true.
 *
 * @param hi2c Pointer to the I2C handle
 */
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c);


/**
 * @brief Callback function for I2C error handling
 *
 * This function is called when an I2C error occurs.
 * It sets the transfer_error flag to true.
 *
 * @param hi2c Pointer to the I2C handle
 */
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c);


/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __INA219_DRIVER_H_ */
