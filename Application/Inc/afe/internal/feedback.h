/*
 * feedback.h
 *
 *  Created on: Dec 30, 2025
 *      Author: ericv
 *
 * Copyright (c) 2025 OpenAquatix Contributors
 * SPDX-License-Identifier: MIT
 */

#ifndef AFE_INTERNAL_FEEDBACK_H_
#define AFE_INTERNAL_FEEDBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include <stdbool.h>

/* Private includes ----------------------------------------------------------*/



/* Exported types ------------------------------------------------------------*/

typedef enum {
  ATTENUATION_93DB,
  ATTENUATION_63DB,
  NUM_IN_FB_ATTENUATIONS
} FeedbackAttenuation_t;

/* Exported constants --------------------------------------------------------*/



/* Exported macro ------------------------------------------------------------*/



/* Exported functions prototypes ---------------------------------------------*/

/**
 * @brief Switches the input feedback op amp on or off
 * 
 * @param on true to turn on, false to turn off
 */
void Feedback_SwitchInput(bool on);

/**
 * @brief Switches the output feedback op amp on or off
 * 
 * @param on true to turn on, false to turn off
 */
void Feedback_SwitchOutput(bool on);

/**
 * @brief Changes the input attenuation by switching the analog switch
 * connected to bottom end of voltage divider
 * 
 * @param attenuation Enum with degree of attenuation
 */
void Feedback_ChangeInputAttenuation(FeedbackAttenuation_t attenuation);

/**
 * @brief Gets the attenuation given an attenuation index
 * 
 * @param attenuation Index of array of attenuations
 * @return float Attenuation in linear terms. Returns 1 if invalid attenuation
 * index
 */
float Feedback_GetAttenuation(FeedbackAttenuation_t attenuation);

/* Private defines -----------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* AFE_INTERNAL_FEEDBACK_H_ */
