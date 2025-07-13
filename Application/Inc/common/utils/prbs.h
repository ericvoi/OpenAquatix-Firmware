/*
 * prbs.h
 *
 *  Created on: Jul 13, 2025
 *      Author: ericv
 */

#ifndef COMMON_UTILS_PRBS_H_
#define COMMON_UTILS_PRBS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void PRBS_Reset();

bool PRBS_GetNext();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_PRBS_H_ */
