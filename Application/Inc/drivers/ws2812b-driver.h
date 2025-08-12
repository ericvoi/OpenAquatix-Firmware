#ifndef __WS2812b_DRIVER_H
#define __WS2812b_DRIVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/

#define WS_TIM			    htim3

/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Initializes the WS2812B LED driver
 *
 * Stops any ongoing PWM/DMA operations, initializes the PWM timer, clears the DMA buffer,
 * and resets all LEDs to off state.
 *
 * @return bool (true if successful)
 *
 * @note This function should be called once during system initialization
 *       before any other WS2812B functions are used.
 */
bool Ws2812b_Init();

/**
 * @brief Sets the RGB color for a specific LED in the strip
 *
 * Updates the internal LED data buffer for the specified LED. The change will not be
 * visible until Ws2812b_Update() is called.
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 *
 * @pre Ws2812b_Init() must have been called successfully
 */
void Ws2812b_SetColour(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Transmits color data to the physical WS2812B LED strip
 *
 * Converts the internal LED data into timed PWM pulses and initiates DMA transfer
 * to drive the LED strip.
 *
 * @param brightness brightness of the colour
 *
 * @return bool true if transfer started successfully,
 *                          false if previous transfer is still in progress
 *
 * @note This function is non-blocking. WS_DMA_COMPLETE_FLAG indicates when
 *       the transfer is finished and a new update can be started.
 */
bool Ws2812b_Update(uint8_t brightness);

/**
 * @brief DMA transfer complete callback for WS2812B LED updates
 *
 * Called automatically when the DMA transfer is complete, typically from
 * a DMA interrupt handler.
 *
 * @note This function should not be called directly from user code.
 */
void WS_Callback();

#ifdef __cplusplus
}
#endif

#endif
