/*
 * WS2812b-driver.c
 *
 *  Created on: Jan 28, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include <string.h>
#include "cfg_defaults.h"
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

static WS_LEDDATARGB led_data;
static WS_LEDDATARGB scaled_led_data;
static uint16_t	ws_dma_buf[WS_DMA_BUF_LEN];
static volatile uint8_t ws_dma_complete_flag;

extern TIM_HandleTypeDef WS_TIM;

void scaleRgb(uint8_t brightness);

/* Exported function prototypes ----------------------------------------------*/

HAL_StatusTypeDef WS_Init()
{
  HAL_TIM_PWM_Stop_DMA(&WS_TIM, WS_TIM_CHANNEL);
  // Initialize PWM timer
  HAL_StatusTypeDef halStatus = HAL_TIM_PWM_Init(&WS_TIM);
  if (halStatus != HAL_OK) return halStatus;

  // Clear DMA buffer
  memset(ws_dma_buf, 0, sizeof(ws_dma_buf));

  // Set DMA Transfer ready flag
  ws_dma_complete_flag = 1;

  WS_SetColour(0, 0, 0);
  WS_Update(255);

  return halStatus;
}

void WS_SetColour(uint8_t r, uint8_t g, uint8_t b)
{
  led_data.colour.r = r;
  led_data.colour.g = g;
  led_data.colour.b = b;
}

HAL_StatusTypeDef WS_Update(uint8_t brightness)
{
  // Check if previous DMA complete
  if (ws_dma_complete_flag == 0) {
    return HAL_BUSY;
  }

  memset(ws_dma_buf, 0, sizeof(ws_dma_buf));

  scaleRgb(brightness);

  uint16_t buf_index = WS_RST_PERIODS;

  uint32_t raw_led_data = scaled_led_data.data;

  for (int8_t byteIndex = 2; byteIndex >= 0; byteIndex--) {
    uint8_t byte = (raw_led_data >> (byteIndex * 8)) & 0xFF;

    // Convert LED data to PWM timing pulses required by WS2812B protocol
    // Each bit becomes a specific pulse width in the DMA buffer
    for (int8_t bit_index = 7; bit_index >= 0; bit_index--) {
      ws_dma_buf[buf_index++] = (byte & (1 << bit_index)) ? WS_HI_VAL : WS_LO_VAL;
    }
  }

  HAL_StatusTypeDef halStatus = HAL_TIM_PWM_Start_DMA(&WS_TIM, WS_TIM_CHANNEL,
      (uint32_t *) ws_dma_buf, WS_DMA_BUF_LEN);

  return halStatus;
}

void WS_Callback()
{
  HAL_TIM_PWM_Stop_DMA(&WS_TIM, WS_TIM_CHANNEL);

  ws_dma_complete_flag = 1;
}

/* Private function prototypes -----------------------------------------------*/

void scaleRgb(uint8_t brightness)
{
  scaled_led_data.colour.r = (uint16_t)led_data.colour.r * brightness / MAX_LED_BRIGHTNESS;
  scaled_led_data.colour.g = (uint16_t)led_data.colour.g * brightness / MAX_LED_BRIGHTNESS;
  scaled_led_data.colour.b = (uint16_t)led_data.colour.b * brightness / MAX_LED_BRIGHTNESS;
}
