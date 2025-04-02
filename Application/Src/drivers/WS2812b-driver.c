/*
 * WS2812b-driver.c
 *
 *  Created on: Jan 28, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include <string.h>
#include "stm32h7xx_hal.h"
#include "WS2812b-driver.h"

/* Private typedef -----------------------------------------------------------*/

typedef union {
	struct {
		uint8_t b;
		uint8_t r;
		uint8_t g;
	} colour;
	uint32_t data;
} WS_LEDDATARGB;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/

static WS_LEDDATARGB WS_LED_DATA[WS_NUM_LEDS];
static uint16_t	WS_DMA_BUF[WS_DMA_BUF_LEN];
static volatile uint8_t WS_DMA_COMPLETE_FLAG;

extern TIM_HandleTypeDef WS_TIM;

/* Private function prototypes -----------------------------------------------*/

HAL_StatusTypeDef WS_Init()
{
  HAL_TIM_PWM_Stop_DMA(&WS_TIM, WS_TIM_CHANNEL);
  // Initialize PWM timer
  HAL_StatusTypeDef halStatus = HAL_TIM_PWM_Init(&WS_TIM);
  if (halStatus != HAL_OK) return halStatus;

  // Clear DMA buffer
  memset(WS_DMA_BUF, 0, sizeof(WS_DMA_BUF));

  // Set DMA Transfer ready flag
  WS_DMA_COMPLETE_FLAG = 1;

  WS_SetColour(0, 0, 0, 0);
  WS_Update();

  return halStatus;
}

void WS_SetColour(uint8_t index, uint8_t r, uint8_t g, uint8_t b)
{
  if (index != 0) {
    return;
  }
  WS_LED_DATA[index].colour.r = r;
  WS_LED_DATA[index].colour.g = g;
  WS_LED_DATA[index].colour.b = b;
}

HAL_StatusTypeDef WS_Update()
{
  // Check if previous DMA complete
  if (WS_DMA_COMPLETE_FLAG == 0) {
    return HAL_BUSY;
  }

  uint16_t bufIndex = WS_RST_PERIODS;

  for (uint8_t ledIndex = 0; ledIndex < WS_NUM_LEDS; ledIndex++) {
    uint32_t ledData = WS_LED_DATA[ledIndex].data;

    for (int8_t byteIndex = 2; byteIndex >= 0; byteIndex--) {
      uint8_t byte = (ledData >> (byteIndex * 8)) & 0xFF;

      // Convert LED data to PWM timing pulses required by WS2812B protocol
      // Each bit becomes a specific pulse width in the DMA buffer
      for (int8_t bit_index = 7; bit_index >= 0; bit_index--) {
        WS_DMA_BUF[bufIndex++] = (byte & (1 << bit_index)) ? WS_HI_VAL : WS_LO_VAL;
      }
    }
  }

  HAL_StatusTypeDef halStatus = HAL_TIM_PWM_Start_DMA(&WS_TIM, WS_TIM_CHANNEL,
      (uint32_t *) WS_DMA_BUF, WS_DMA_BUF_LEN);

  return halStatus;
}

void WS_Callback()
{
  HAL_TIM_PWM_Stop_DMA(&WS_TIM, WS_TIM_CHANNEL);

  WS_DMA_COMPLETE_FLAG = 1;
}
