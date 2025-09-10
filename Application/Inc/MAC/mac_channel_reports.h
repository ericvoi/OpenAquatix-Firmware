/*
 * mac_channel_reports.h
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

#ifndef MAC_MAC_CHANNEL_REPORTS_H_
#define MAC_MAC_CHANNEL_REPORTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  float psd;
} ChannelReport_t;

/* Exported constants --------------------------------------------------------*/

#define CHANNEL_REPORT_CD             16 // Number of chip periods in a channel report given to MAC task

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_CHANNEL_REPORTS_H_ */
