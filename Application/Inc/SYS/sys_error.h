/*
 * sys_error.h
 *
 *  Created on: Mar 11, 2025
 *      Author: ericv
 */

#ifndef SYS_SYS_ERROR_H_
#define SYS_SYS_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  ERROR_CFG_INIT,
  ERROR_COMM_INIT,
  ERROR_MESS_INIT,
  ERROR_SYS_INIT,
  ERROR_MESS_PROCESSING,
  ERROR_OTHER
} ErrorCodes_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void Error_Routine(ErrorCodes_t error_code);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SYS_SYS_ERROR_H_ */
