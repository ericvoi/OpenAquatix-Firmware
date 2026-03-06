/*
 * error_subsys.c
 *
 *  Created on: Mar 6, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "error_subsys.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef struct {
  bool is_disabled;
  bool pending_reset;
} SubSystemInfo_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static SubSystemInfo_t subsys_info[NUM_SUBSYS] = {0};

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void ErrorSubsys_Disable(SubSystemId_t subsys)
{
  if (subsys > NUM_SUBSYS) return;
  subsys_info[subsys].is_disabled = true;
}

void ErrorSubsys_RequestReset(SubSystemId_t subsys)
{
  if (subsys > NUM_SUBSYS) return;
  subsys_info[subsys].pending_reset = true;
}

SubSystemStatus_t ErrorSubsys_CurrentStatus(SubSystemId_t subsys)
{
  if (subsys > NUM_SUBSYS) return SUBSYS_PROCEED;
  if (subsys_info[subsys].is_disabled) return SUBSYS_DISABLE;
  if (subsys_info[subsys].pending_reset) return SUBSYS_RESET;

  return SUBSYS_PROCEED;
}

void ErrorSubsys_ClearReset(SubSystemId_t subsys)
{
  subsys_info[subsys].pending_reset = false;
}

/* Private function definitions ----------------------------------------------*/
