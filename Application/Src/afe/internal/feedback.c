/*
 * feedback.c
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "stm32h7xx_hal.h"
#include "main.h"
#include "internal/feedback.h"
#include "pwr_domains.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define ATTEN_R1        (1.0E6)
#define ATTEN_R2A       (22.0f)
#define ATTEN_R2B       (732.0f)

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static float in_fb_attenuations[NUM_IN_FB_ATTENUATIONS] = {
  ATTEN_R2A / (ATTEN_R2A + ATTEN_R1),
  ATTEN_R2B / (ATTEN_R2B + ATTEN_R1)
};

/* Private function prototypes -----------------------------------------------*/



/* Exported function definitions ---------------------------------------------*/

void Feedback_SwitchInput(bool on)
{
  HAL_GPIO_WritePin(INPUT_FB_EN_GPIO_Port, INPUT_FB_EN_Pin, ! on);
}

void Feedback_SwitchOutput(bool on)
{
  HAL_GPIO_WritePin(OUTPUT_FB_EN__GPIO_Port, OUTPUT_FB_EN__Pin, ! on);
}

void Feedback_ChangeInputAttenuation(FeedbackAttenuation_t attenuation)
{
  switch (attenuation) {
    case ATTENUATION_93DB:
      HAL_GPIO_WritePin(FBK_ATTENUATION_GPIO_Port, FBK_ATTENUATION_Pin, GPIO_PIN_RESET);
      break;
    case ATTENUATION_63DB:
      HAL_GPIO_WritePin(FBK_ATTENUATION_GPIO_Port, FBK_ATTENUATION_Pin, GPIO_PIN_SET);
      break;
    default:
      // TODO: handle error
      break;
  }
}

float Feedback_GetAttenuation(FeedbackAttenuation_t attenuation)
{
  if (attenuation > NUM_IN_FB_ATTENUATIONS) return 1;

  return in_fb_attenuations[attenuation];
}

/* Private function definitions ----------------------------------------------*/
