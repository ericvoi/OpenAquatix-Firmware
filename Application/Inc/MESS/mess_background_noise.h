/*
 * mess_background_noise.h
 *
 *  Created on: Jul 14, 2025
 *      Author: ericv
 */

#ifndef MESS_MESS_BACKGROUND_NOISE_H_
#define MESS_MESS_BACKGROUND_NOISE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

void BackgroundNoise_Reset();
void BackgroundNoise_Calculate();
float BackgroundNoise_Get();
bool BackgroundNoise_Ready();

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* MESS_MESS_BACKGROUND_NOISE_H_ */
