/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "cmsis_os.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

// Whenever a new task is added, an enum must be added here
typedef enum {
  CFG_TASK,
  COMM_TASK,
  MESS_TASK,
  SYS_TASK,
  DAC_TASK,
  MAC_TASK,
  FILT_TASK,
  NUM_TASKS
} TaskIds_t;

typedef enum {
  SUBSYS_POWER,
  SUBSYS_PGA,
  SUBSYS_FBK_TESTS,
  SUBSYS_LPS,
  SUBSYS_INA,
  SUBSYS_TJ,          // Junction temperature
  NUM_SUBSYS,
  SUBSYS_NONE // Used for error manager when an error is not tied to a subsystem
} SubSystemId_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern osEventFlagsId_t print_event_handle;
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint32_t HAL_TickRolloverCount(void);
uint64_t HAL_AbsoluteTimestamp(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define EN_3V3A_LED_Pin GPIO_PIN_2
#define EN_3V3A_LED_GPIO_Port GPIOE
#define EN_3V3_LED_Pin GPIO_PIN_3
#define EN_3V3_LED_GPIO_Port GPIOE
#define PGOOD_5V_Pin GPIO_PIN_5
#define PGOOD_5V_GPIO_Port GPIOE
#define ULPI_RST__Pin GPIO_PIN_6
#define ULPI_RST__GPIO_Port GPIOE
#define MAIN_TO_DAU_Pin GPIO_PIN_15
#define MAIN_TO_DAU_GPIO_Port GPIOC
#define EN_30V_Pin GPIO_PIN_1
#define EN_30V_GPIO_Port GPIOC
#define PGOOD_30V_Pin GPIO_PIN_0
#define PGOOD_30V_GPIO_Port GPIOA
#define TPA_FAULT_Pin GPIO_PIN_2
#define TPA_FAULT_GPIO_Port GPIOA
#define ADC2_3_RECV_Pin GPIO_PIN_6
#define ADC2_3_RECV_GPIO_Port GPIOA
#define DAC_SEL_Pin GPIO_PIN_7
#define DAC_SEL_GPIO_Port GPIOA
#define TPA_CLIP_OTW_Pin GPIO_PIN_5
#define TPA_CLIP_OTW_GPIO_Port GPIOC
#define TPA_RESET_Pin GPIO_PIN_13
#define TPA_RESET_GPIO_Port GPIOE
#define PAMP_FILTER_EN_Pin GPIO_PIN_15
#define PAMP_FILTER_EN_GPIO_Port GPIOE
#define OUTPUT_FB_EN__Pin GPIO_PIN_14
#define OUTPUT_FB_EN__GPIO_Port GPIOB
#define INPUT_FB_EN_Pin GPIO_PIN_15
#define INPUT_FB_EN_GPIO_Port GPIOB
#define FBK_ATTENUATION_Pin GPIO_PIN_8
#define FBK_ATTENUATION_GPIO_Port GPIOD
#define TR_SEL_Pin GPIO_PIN_9
#define TR_SEL_GPIO_Port GPIOD
#define TR_EN_Pin GPIO_PIN_10
#define TR_EN_GPIO_Port GPIOD
#define EN_3V3A_Pin GPIO_PIN_11
#define EN_3V3A_GPIO_Port GPIOD
#define EN__5V_Pin GPIO_PIN_12
#define EN__5V_GPIO_Port GPIOD
#define LED_DATA_Pin GPIO_PIN_6
#define LED_DATA_GPIO_Port GPIOC
#define WS_EN_Pin GPIO_PIN_7
#define WS_EN_GPIO_Port GPIOC
#define EN__3V3_Pin GPIO_PIN_8
#define EN__3V3_GPIO_Port GPIOC
#define LPS_INT_DRDY_Pin GPIO_PIN_0
#define LPS_INT_DRDY_GPIO_Port GPIOD
#define LPS_INT_DRDY_EXTI_IRQn EXTI0_IRQn
#define HW_ID_PIN0_Pin GPIO_PIN_4
#define HW_ID_PIN0_GPIO_Port GPIOD
#define HW_ID_PIN1_Pin GPIO_PIN_5
#define HW_ID_PIN1_GPIO_Port GPIOD
#define HW_ID_PIN2_Pin GPIO_PIN_6
#define HW_ID_PIN2_GPIO_Port GPIOD
#define HW_ID_PIN3_Pin GPIO_PIN_7
#define HW_ID_PIN3_GPIO_Port GPIOD
#define EN_30V_LED_Pin GPIO_PIN_0
#define EN_30V_LED_GPIO_Port GPIOE
#define EN_BATT_LED_Pin GPIO_PIN_1
#define EN_BATT_LED_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
