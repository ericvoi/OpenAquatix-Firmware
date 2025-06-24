/*
 * mess_sync.h
 *
 *  Created on: Jun 22, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_SYNC_H_
#define MESS_MESS_SYNC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mess_dsp_config.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

bool Sync_Add(const DspConfig_t* cfg);
bool Sync_Synchronize(const DspConfig_t* cfg);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_SYNC_H_ */
