#ifndef __WS2812b_DRIVER_H
#define __WS2812b_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"


/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the WS2812B LED driver
 *
 * Stops any ongoing PWM/DMA operations, initializes the PWM timer, clears the DMA buffer,
 * and resets all LEDs to off state.
 *
 * @return HAL_StatusTypeDef HAL status (HAL_OK if successful)
 *
 * @note This function should be called once during system initialization
 *       before any other WS2812B functions are used.
 */
HAL_StatusTypeDef WS_Init();

/**
 * @brief Sets the RGB color for a specific LED in the strip
 *
 * Updates the internal LED data buffer for the specified LED. The change will not be
 * visible until WS_Update() is called.
 *
 * @param index Zero-based index of the LED to set
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 *
 * @pre WS_Init() must have been called successfully
 * @warning Ensure index is less than WS_NUM_LEDS to avoid buffer overrun
 */
void WS_SetColour(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Transmits color data to the physical WS2812B LED strip
 *
 * Converts the internal LED data into timed PWM pulses and initiates DMA transfer
 * to drive the LED strip.
 *
 * @return HAL_StatusTypeDef HAL_OK if transfer started successfully,
 *                          HAL_BUSY if previous transfer is still in progress
 *
 * @note This function is non-blocking. WS_DMA_COMPLETE_FLAG indicates when
 *       the transfer is finished and a new update can be started.
 */
HAL_StatusTypeDef WS_Update();

/**
 * @brief DMA transfer complete callback for WS2812B LED updates
 *
 * Called automatically when the DMA transfer is complete, typically from
 * a DMA interrupt handler.
 *
 * @note This function should not be called directly from user code.
 */
void WS_Callback();

/* Private defines -----------------------------------------------------------*/

#define WS_NUM_LEDS 	  1

#define WS_TIM			    htim3
#define WS_TIM_CHANNEL	TIM_CHANNEL_1

// Assuming 800 kHz and ARR of 100
#define WS_HI_VAL		    67	// 0.8 us
#define WS_LO_VAL		    33  // 0.4 us

#define WS_RST_PERIODS	50	// 62.5 us. 50 us required for reset
#define WS_BITS_PER_LED	24

#define WS_DMA_BUF_LEN	((WS_NUM_LEDS * WS_BITS_PER_LED) + WS_RST_PERIODS * 2)

#ifdef __cplusplus
}
#endif

#endif
