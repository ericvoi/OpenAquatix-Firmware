/*
 * afe.c
 *
 *  Created on: Dec 31, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

/* Private includes ----------------------------------------------------------*/

#include "stm32h723xx.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"
#include "afe.h"
#include "main.h"
#include "internal/feedback.h"
#include "internal/dac_switch.h"
#include "internal/tpa32xx-driver.h"
#include "internal/tr_switch.h"
#include "pwr_domains.h"
#include "mess_filt_resources.h"
#include "mess_input.h"
#include "mess_sync.h"
#include "error_manager.h"
#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/



/* Private define ------------------------------------------------------------*/

#define TRANSDUCER_SETTLE_TIME_MS         200
#define TRANSITION_TIMEOUT_MS             750

/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static AfeMode_t afe_mode;

extern TIM_HandleTypeDef htim6;
extern DAC_HandleTypeDef hdac1;

/* Private function prototypes -----------------------------------------------*/

static void enterIdle(void);
static void enterRx(void);
static void enterRxFeedback(void);
static void enterTx(bool with_feedback);

static bool isTimedOut(uint64_t start_time);
static void restartADCs(void);

/* Exported function definitions ---------------------------------------------*/

void AFE_Init()
{
  enterIdle();
}

void AFE_SetMode(AfeMode_t new_mode)
{
  Feedback_ChangeInputAttenuation(ATTENUATION_63DB);
  switch (new_mode) {
    case AFE_MODE_IDLE:
      enterIdle();
      break;
    case AFE_MODE_RX:
      enterRx();
      break;
    case AFE_MODE_RX_FEEDBACK:
      enterRxFeedback();
      break;
    case AFE_MODE_TX:
      enterTx(false);
      break;
    case AFE_MODE_TX_FEEDBACK:
      enterTx(true);
      break;
    default:
      REGISTER_ERROR(ERROR_UNHANDLED_CASE);
  }
}

AfeMode_t AFE_GetMode(void)
{
  return afe_mode;
}

bool AFE_IsTransmitting(void)
{
  return (afe_mode == AFE_MODE_TX) || (afe_mode == AFE_MODE_TX_FEEDBACK);
}

bool AFE_IsReceiving(void)
{
  return (afe_mode == AFE_MODE_RX) || (afe_mode == AFE_MODE_RX_FEEDBACK);
}

/* Private function definitions ----------------------------------------------*/

void enterIdle(void)
{
  uint64_t start_timestamp = HAL_AbsoluteTimestamp();
  Feedback_SwitchInput(false);
  Feedback_SwitchOutput(false);
  PWR_Analog(false);
  PWR_30V(false);
  TR_Change(TR_NONE);
  // No need to change anything with the dac switch or the tpa since they are
  // powered off and I/Os protected
  while (PWR_State30V() != PWR_OFF && PWR_StateAnalog() != PWR_OFF) {
    if (isTimedOut(start_timestamp)) 
      REGISTER_ERROR(ERROR_AFE_TIMEOUT);
    
    osDelay(1);
  }
  afe_mode = AFE_MODE_IDLE;
}

void enterRx(void)
{
  uint64_t start_timestamp = HAL_AbsoluteTimestamp();
  HAL_TIM_Base_Stop(&htim6);
  HAL_DAC_Stop(&hdac1, DAC_CHANNEL_1);
  Feedback_SwitchInput(false);
  Feedback_SwitchOutput(false);
  PWR_Analog(true);
  PWR_30V(false);
  if (AFE_IsTransmitting()) {
    osDelay(TRANSDUCER_SETTLE_TIME_MS);
  }
  osDelay(3); // TR relay settle (~3 ms per datasheet)
  while (PWR_State30V() != PWR_OFF || PWR_StateAnalog() != PWR_READY) {
    if (isTimedOut(start_timestamp))
      REGISTER_ERROR(ERROR_AFE_TIMEOUT);

    osDelay(1);
  }
  TR_Change(TR_INPUT_MODE);
  // DAC switch does not matter since no modulation
  // TPA not configured since powered off by no 30V
  RETURN_IF_ERROR_PRESENT(restartADCs());
  
  afe_mode = AFE_MODE_RX;
}

void enterRxFeedback(void)
{
  uint64_t start_timestamp = HAL_AbsoluteTimestamp();
  Feedback_SwitchInput(true);
  Feedback_SwitchOutput(false);
  PWR_Analog(true);
  PWR_30V(false);
  TR_Change(TR_NONE);
  DACSwitch_Change(DAC_DIRECTION_FEEDBACK);
  if (AFE_IsTransmitting()) {
    osDelay(TRANSDUCER_SETTLE_TIME_MS);
  }
  osDelay(3); // TR relay settle (~3 ms per datasheet)
  // TPA configuration does not matter since powered off
  while (PWR_State30V() != PWR_OFF || PWR_StateAnalog() != PWR_READY) {
    if (isTimedOut(start_timestamp))
      REGISTER_ERROR(ERROR_AFE_TIMEOUT);

    osDelay(1);
  }
  RETURN_IF_ERROR_PRESENT(restartADCs());
  afe_mode = AFE_MODE_RX_FEEDBACK;
}

void enterTx(bool with_feedback)
{
  uint64_t start_timestamp = HAL_AbsoluteTimestamp();
  if (MessFiltResources_StopAllAdcs() == false) {
    REGISTER_ERROR(ERROR_AFE_GENERAL);
  }
  Feedback_SwitchInput(false);
  Feedback_SwitchOutput(with_feedback);
  TPA_Mute(); // Mute PA prior to voltage rails being turned on to turn on in defined state
  PWR_Analog(true);
  PWR_30V(true);

  // Setting the DAC to mid-value prevents high frequency spike when DAC starts modulating
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
  HAL_TIM_Base_Start(&htim6);
  HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 2048);
  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);

  osDelay(1);
  HAL_TIM_Base_Stop(&htim6);
  while (PWR_State30V() != PWR_READY || PWR_StateAnalog() != PWR_READY) {
    if (isTimedOut(start_timestamp))
      REGISTER_ERROR(ERROR_AFE_TIMEOUT);

    osDelay(1);
  }
  TPA_Unmute();
  TR_Change(TR_OUTPUT_MODE);
  DACSwitch_Change(DAC_DIRECTION_TRANSDUCER);
  osDelay(3); // TR relay settle (~3 ms per datasheet)
  if (with_feedback == true) {
    afe_mode = AFE_MODE_TX_FEEDBACK;
  }
  else {
    afe_mode = AFE_MODE_TX;
  }
}

bool isTimedOut(uint64_t start_time)
{
  return (HAL_AbsoluteTimestamp() - start_time) > TRANSITION_TIMEOUT_MS;
}

// Function only called when transitioning to receiving state
void restartADCs()
{
  if (MessFiltResources_StopAllAdcs() == false) {
    REGISTER_ERROR(ERROR_AFE_GENERAL);
  }
  Input_Reset();
  MessFiltResources_StartInputAdc();
  // If receiving prior, then the sync state is fine, but not if transmitting prior
  // Reset not used if not needed since computationally expensive
  if (AFE_IsTransmitting()) 
    Sync_Reset();
}
