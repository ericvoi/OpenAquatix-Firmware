/*
 * bkpsram_layout.h
 *
 *  Created on: Feb 23, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef BSP_BKPSRAM_LAYOUT_H_
#define BSP_BKPSRAM_LAYOUT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "error_log.h"

/* Private includes ----------------------------------------------------------*/

typedef struct {
  // WARNING: magic_number should never be moved!
  uint32_t magic_number;    // Bootloader magic number
  uint32_t reset_count;     // Resets since last POR
  uint32_t log_reset_flag;  // Flag indicating if the log should be reset
  ErrorEntry_t error_log[MAX_ENTRIES_IN_ERROR_LOG]; // Persistent error log
} BkpSramData_t;

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define LOG_RESET_MAGIC_NUMBER            0x12983476

/* Exported macro ------------------------------------------------------------*/



/* Exported variables --------------------------------------------------------*/

extern volatile BkpSramData_t bkpsram;

/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Resets the backup SRAM to 0 when the reset context is a POR. 
 * 
 * @note NEVER reference any global or stack variables besides bkpsram when 
 * modifying this function
 * 
 * @note Only call this function from the reset sequence
 */
void Bkpsram_Init(void);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* BSP_BKPSRAM_LAYOUT_H_ */
