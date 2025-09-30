/*
 * mac_no_mac.h
 *
 *  Created on: Sep 9, 2025
 *      Author: ericv
 */

#ifndef MAC_MAC_NO_MAC_H_
#define MAC_MAC_NO_MAC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "mac_main.h"
#include <stdint.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef struct {
  uint32_t channel_reserved_until;
  bool active_reservation;
} NoMacData_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/



/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MAC_MAC_NO_MAC_H_ */
