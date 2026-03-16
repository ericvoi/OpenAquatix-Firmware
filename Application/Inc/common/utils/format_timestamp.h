/*
 * format_timestamp.h
 *
 *  Created on: Feb 24, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef COMMON_UTILS_FORMAT_TIMESTAMP_H_
#define COMMON_UTILS_FORMAT_TIMESTAMP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

/**
 * Timestamp format:
 * 
 * {Resets}:{days}:{hours}:{minutes}:{seconds}:{milliseconds}
 * 
 * RRRRR:DDDD:HH:MM:SS:mmm
 * 
 */
#define TIMESTAMP_RESET_CHARS                   5
#define TIMESTAMP_DAY_CHARS                     4
#define TIMESTAMP_HOUR_CHARS                    2
#define TIMESTAMP_MINUTE_CHARS                  2
#define TIMESTAMP_SECOND_CHARS                  2
#define TIMESTAMP_MS_CHARS                      3
#define TIMESTAMP_COLONS_REQUIRED               5
#define FORMATTED_TIMESTAMP_SIZE               (TIMESTAMP_RESET_CHARS + \
                                                TIMESTAMP_DAY_CHARS + \
                                                TIMESTAMP_HOUR_CHARS + \
                                                TIMESTAMP_MINUTE_CHARS + \
                                                TIMESTAMP_SECOND_CHARS + \
                                                TIMESTAMP_MS_CHARS + \
                                                TIMESTAMP_COLONS_REQUIRED + \
                                                1) // NULL Terminator

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void formatAbsoluteTimestamp(uint16_t reset_count, uint64_t absolute_timestamp, char out[FORMATTED_TIMESTAMP_SIZE]);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* COMMON_UTILS_FORMAT_TIMESTAMP_H_ */
