/*
 * format_timestamp.c
 *
 *  Created on: Feb 24, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "format_timestamp.h"
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void formatAbsoluteTimestamp(uint16_t reset_count, uint64_t absolute_timestamp, char out[FORMATTED_TIMESTAMP_SIZE])
{
  uint32_t ms      = (uint32_t)(absolute_timestamp % 1000ULL);
  uint32_t seconds = (uint32_t)((absolute_timestamp / 1000ULL)     % 60ULL);
  uint32_t minutes = (uint32_t)((absolute_timestamp / 60000ULL)    % 60ULL);
  uint32_t hours   = (uint32_t)((absolute_timestamp / 3600000ULL)  % 24ULL);
  uint32_t days    = (uint32_t) (absolute_timestamp / 86400000ULL);

  snprintf(out, FORMATTED_TIMESTAMP_SIZE,
           "%05u:%04u:%02u:%02u:%02u:%03u",
           (unsigned int)reset_count,
           (unsigned int)days,
           (unsigned int)hours,
           (unsigned int)minutes,
           (unsigned int)seconds,
           (unsigned int)ms);
}

/* Private function definitions ----------------------------------------------*/
