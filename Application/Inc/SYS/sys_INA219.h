// sys_INA219.h

#ifndef __SYS_INA219_H_
#define __SYS_INA219_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>
#include "INA219-driver.h"  // Get access to INA219 constants

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define INA_PERIOD_MS 1 // Power reading every 1ms


/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool INA219_System_Init(void);
void INA219_Timer_Callback(void);
void INA219_ReadComplete_Callback(bool success);


#ifdef __cplusplus
}
#endif

#endif /* __SYS_INA219_H_ */