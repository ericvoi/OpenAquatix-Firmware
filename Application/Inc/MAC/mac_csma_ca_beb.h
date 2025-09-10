/*
 * mac_csma_ca_beb.h
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

#ifndef MAC_MAC_CSMA_CA_BEB_H_
#define MAC_MAC_CSMA_CA_BEB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "mac_channel_reports.h"
#include "mac_main.h"
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define CSMA_SLOT_LENGTH_CD                   176
#define CSMA_BACKGROUND_NOISE_ESTIMATION_CD   352

#define CSMA_REPORTS_IN_SLOT                  (CSMA_SLOT_LENGTH_CD / CHANNEL_REPORT_CD)
#define CSMA_REPORTS_IN_BACKGROUND_NOISE      (CSMA_BACKGROUND_NOISE_ESTIMATION_CD / CHANNEL_REPORT_CD)

typedef struct {
  MacState_t state;

  uint8_t collision_count;
  
  float background_psd;
  float channel_reports[CSMA_REPORTS_IN_BACKGROUND_NOISE];
  uint16_t num_channel_reports;
  uint16_t channel_report_index;
  bool sensing_complete;
} CsmaCaBebData_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_CSMA_CA_BEB_H_ */
