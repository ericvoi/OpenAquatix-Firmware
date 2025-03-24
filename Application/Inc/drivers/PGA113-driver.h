/*
 * PGA113-driver.h
 *
 *  Created on: Jan 31, 2025
 *      Author: ericv
 */

#ifndef __PGA113_DRIVER_H_
#define __PGA113_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  PGA_GAIN_1,
  PGA_GAIN_2,
  PGA_GAIN_5,
  PGA_GAIN_10,
  PGA_GAIN_20,
  PGA_GAIN_50,
  PGA_GAIN_100,
  PGA_GAIN_200
} PGA_Gain_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

HAL_StatusTypeDef PGA_Init();
void              PGA_SetGain(PGA_Gain_t gain);
HAL_StatusTypeDef PGA_Read();
HAL_StatusTypeDef PGA_Update();
HAL_StatusTypeDef PGA_Shutdown();
HAL_StatusTypeDef PGA_Enable();
HAL_StatusTypeDef PGA_Status();
uint8_t           PGA_GetGain();


/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __PGA113_DRIVER_H_ */
