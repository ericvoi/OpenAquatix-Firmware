/*
 * usb_main.c
 *
 *  Created on: Mar 31, 2026
 *      Author: ericv
 *
 * Copyright (c) 2026 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "usb_main.h"
#include "hmi_usb.h"
#include "error_manager.h"
#include "cfg_main.h"
#include "tusb.h"

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/



/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void USB_StartTask(void* argument)
{
  (void)(argument);
  USB_CreateShared();
  tusb_init();

  Error_RegisterTask("USB");
  Error_ParameterRegistrationComplete();
  
  CFG_WaitLoadComplete();

  for(;;)
  {
    tud_task();
  }
}

/* Private function definitions ----------------------------------------------*/
