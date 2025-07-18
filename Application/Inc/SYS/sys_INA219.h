// sys_INA219.h

#ifndef __SYS_INA219_H_
#define __SYS_INA219_H_

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

HAL_StatusTypeDef SYS_INA219_ReadCurrent(float *current);
HAL_StatusTypeDef SYS_INA219_ReadVoltage(float *voltage);
HAL_StatusTypeDef SYS_INA219_ReadPower(float *power);

#ifdef __cplusplus
}
#endif

#endif /* __SYS_INA219_H_ */