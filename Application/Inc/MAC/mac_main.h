/*
 * mac_main.h
 *
 *  Created on: Sep 8, 2025
 *      Author: ericv
 */

#ifndef MAC_MAC_MAIN_H_
#define MAC_MAC_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/



/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  MAC_STATE_IDLE,
  MAC_STATE_WAITING_TO_TRANSMIT,
  MAC_STATE_TRANSMITTED
} MacState_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void MAC_StartTask(void* argument);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_MAIN_H_ */
