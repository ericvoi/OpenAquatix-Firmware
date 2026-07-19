/*
 * janus_utils.c
 *
 *  Created on: Jul 16, 2026
 *      Author: ericv
 * 
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "janus_utils.h"
#include "error_manager.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

uint16_t encodeJanusReservationTime(uint32_t time_ms)
{
  uint16_t reservation_time_index = (uint16_t) ceilf(time_ms * 25.0f / 4.0f + 14.0f);

  uint8_t e = 0;
  while ((15 + 16) * (1 << e) < reservation_time_index) {
    e++;
    if (e > 7) REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);
  }
  uint8_t m = (uint8_t) ceil(((float) reservation_time_index) / ((float) (1 << e)) - 16.0f);
  if (m > 15)
    REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);
  return ((e & 0x07) << 4) | (m & 0x0F);
}

uint32_t decodeJanusReservationTime(uint16_t coded_value)
{
  if (coded_value >= (1 << JANUS_RESERVATION_BITS))
    REGISTER_ERROR_NON_VOID(ERROR_INVALID_RESERVATION_TIME, 0);

  uint8_t m = coded_value & 0x0F;
  uint8_t e = (coded_value >> 4) & 0x07;
  float reservation_time = ((m + 16.0f) * (1 << e) - 14.0f) * (4.0f / 25.0f);
  return (uint32_t) (reservation_time * 1000.0f);
}

/* Private function definitions ----------------------------------------------*/
