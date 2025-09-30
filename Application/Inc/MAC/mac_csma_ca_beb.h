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
#include "cfg_defaults.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

#define CSMA_SLOT_LENGTH_CD                   176
#define CSMA_BACKGROUND_NOISE_ESTIMATION_CD   352

#define CSMA_REPORTS_IN_SLOT                  (CSMA_SLOT_LENGTH_CD / CHANNEL_REPORT_CD)
#define CSMA_REPORTS_IN_BACKGROUND_NOISE      (CSMA_BACKGROUND_NOISE_ESTIMATION_CD / CHANNEL_REPORT_CD)

#define STORED_CSMA_CHANNEL_REPORTS           (CSMA_REPORTS_IN_BACKGROUND_NOISE + 5)

typedef enum {
  NO_BACKOFF,
  CONTINUOUS_BACKOFF,
  SLOT_BACKOFF
} BackoffState_t;

typedef struct {
  BackoffState_t backoff_state;
  uint8_t C;
  uint16_t reports_remaining_in_slot;
  bool busy_slot;
  
  float background_psd;
  float channel_reports[STORED_CSMA_CHANNEL_REPORTS];
  uint16_t num_channel_reports;
  uint16_t channel_report_index;
  bool sensing_complete;

  bool last_report_busy;
  bool fresh_report;

  uint32_t channel_reserved_until;
  bool active_reservation;

  bool message_deferred;
  uint32_t deferral_timestamp; // Watchdog timestamp that ensures the mac task does not get stuck trying to send a message when there is no channel information available
} CsmaCaBebData_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_CSMA_CA_BEB_H_ */
