/*
 * sys_pressure.c
 *
 *  Created on: Jan 4, 2026
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "sys_pressure.h"
#include "lps22hh-driver.h"
#include <stdint.h>
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define PRESSURE_BUFFER_SIZE      16

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static uint32_t p_buf[PRESSURE_BUFFER_SIZE];
static uint16_t p_buf_head = 0;
static uint16_t p_buf_tail = 0;

static float current_pressure_hpa = 0.0f;

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

bool Pressure_Init(void)
{
  p_buf_head = 0;
  p_buf_tail = 0;
  if (LPS_RegisterPressureBuf(p_buf, PRESSURE_BUFFER_SIZE, &p_buf_head) == false) {
    return false;
  }
}

void Pressure_Process(void)
{
  while (p_buf_head != p_buf_tail) {
    current_pressure_hpa = LPS_ConvertRawPressure(p_buf[p_buf_tail]);
    p_buf_tail = (p_buf_tail + 1) % PRESSURE_BUFFER_SIZE;
  }
}

float Pressure_GetCurrent(void)
{
  return current_pressure_hpa;
}

/* Private function definitions ----------------------------------------------*/
