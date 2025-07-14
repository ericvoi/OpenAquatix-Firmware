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
typedef enum {
  CFG_TASK,
  COMM_TASK,
  MESS_TASK,
  SYS_TASK,
  DAC_TASK,
  NUM_TASKS
} TaskIds_t;
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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define UNUSED1_Pin GPIO_PIN_2
#define UNUSED1_GPIO_Port GPIOE
#define UNUSED2_Pin GPIO_PIN_3
#define UNUSED2_GPIO_Port GPIOE
#define UNUSED3_Pin GPIO_PIN_4
#define UNUSED3_GPIO_Port GPIOE
#define UNUSED4_Pin GPIO_PIN_5
#define UNUSED4_GPIO_Port GPIOE
#define UNUSED5_Pin GPIO_PIN_6
#define UNUSED5_GPIO_Port GPIOE
#define UNUSED6_Pin GPIO_PIN_13
#define UNUSED6_GPIO_Port GPIOC
#define UNUSED7_Pin GPIO_PIN_14
#define UNUSED7_GPIO_Port GPIOC
#define UNUSED8_Pin GPIO_PIN_15
#define UNUSED8_GPIO_Port GPIOC
#define UNUSED9_Pin GPIO_PIN_0
#define UNUSED9_GPIO_Port GPIOC
#define UNUSED10_Pin GPIO_PIN_1
#define UNUSED10_GPIO_Port GPIOC
#define UNUSED11_Pin GPIO_PIN_2
#define UNUSED11_GPIO_Port GPIOC
#define ADC3_1_RECV_Pin GPIO_PIN_3
#define ADC3_1_RECV_GPIO_Port GPIOC
#define UNUSED12_Pin GPIO_PIN_0
#define UNUSED12_GPIO_Port GPIOA
#define UNUSED13_Pin GPIO_PIN_1
#define UNUSED13_GPIO_Port GPIOA
#define UNUSED14_Pin GPIO_PIN_2
#define UNUSED14_GPIO_Port GPIOA
#define UNUSED15_Pin GPIO_PIN_3
#define UNUSED15_GPIO_Port GPIOA
#define DAC1_1_OUT_Pin GPIO_PIN_4
#define DAC1_1_OUT_GPIO_Port GPIOA
#define DAC1_2_FB_Pin GPIO_PIN_5
#define DAC1_2_FB_GPIO_Port GPIOA
#define ADC2_3_RECV_Pin GPIO_PIN_6
#define ADC2_3_RECV_GPIO_Port GPIOA
#define UNUSED16_Pin GPIO_PIN_7
#define UNUSED16_GPIO_Port GPIOA
#define UNUSED17_Pin GPIO_PIN_4
#define UNUSED17_GPIO_Port GPIOC
#define UNUSED18_Pin GPIO_PIN_5
#define UNUSED18_GPIO_Port GPIOC
#define UNUSED19_Pin GPIO_PIN_0
#define UNUSED19_GPIO_Port GPIOB
#define ADC1_5_FB_Pin GPIO_PIN_1
#define ADC1_5_FB_GPIO_Port GPIOB
#define PAMP_FAULTZ_Pin GPIO_PIN_2
#define PAMP_FAULTZ_GPIO_Port GPIOB
#define PAMP_FAULTZ_EXTI_IRQn EXTI2_IRQn
#define PAMP_MUTE_Pin GPIO_PIN_7
#define PAMP_MUTE_GPIO_Port GPIOE
#define PGOOD_26V_Pin GPIO_PIN_8
#define PGOOD_26V_GPIO_Port GPIOE
#define UNUSED20_Pin GPIO_PIN_9
#define UNUSED20_GPIO_Port GPIOE
#define UNUSED21_Pin GPIO_PIN_10
#define UNUSED21_GPIO_Port GPIOE
#define SPI4_NCS_Pin GPIO_PIN_11
#define SPI4_NCS_GPIO_Port GPIOE
#define UNUSED22_Pin GPIO_PIN_13
#define UNUSED22_GPIO_Port GPIOE
#define UNUSED23_Pin GPIO_PIN_15
#define UNUSED23_GPIO_Port GPIOE
#define UNUSED24_Pin GPIO_PIN_10
#define UNUSED24_GPIO_Port GPIOB
#define UNUSED25_Pin GPIO_PIN_11
#define UNUSED25_GPIO_Port GPIOB
#define UART5_RX_Pin GPIO_PIN_12
#define UART5_RX_GPIO_Port GPIOB
#define GPIO_DAU_TO_MAIN_Pin GPIO_PIN_14
#define GPIO_DAU_TO_MAIN_GPIO_Port GPIOB
#define GPIO_MAIN_TO_DAU_Pin GPIO_PIN_15
#define GPIO_MAIN_TO_DAU_GPIO_Port GPIOB
#define TR_CTRL_Pin GPIO_PIN_8
#define TR_CTRL_GPIO_Port GPIOD
#define UNUSED26_Pin GPIO_PIN_9
#define UNUSED26_GPIO_Port GPIOD
#define UNUSED27_Pin GPIO_PIN_10
#define UNUSED27_GPIO_Port GPIOD
#define UNUSED28_Pin GPIO_PIN_11
#define UNUSED28_GPIO_Port GPIOD
#define UNUSED29_Pin GPIO_PIN_12
#define UNUSED29_GPIO_Port GPIOD
#define UNUSED30_Pin GPIO_PIN_13
#define UNUSED30_GPIO_Port GPIOD
#define UNUSED31_Pin GPIO_PIN_14
#define UNUSED31_GPIO_Port GPIOD
#define UNUSED32_Pin GPIO_PIN_15
#define UNUSED32_GPIO_Port GPIOD
#define LED_DATA_Pin GPIO_PIN_6
#define LED_DATA_GPIO_Port GPIOC
#define UNUSED33_Pin GPIO_PIN_7
#define UNUSED33_GPIO_Port GPIOC
#define EN_N3V3_Pin GPIO_PIN_8
#define EN_N3V3_GPIO_Port GPIOC
#define UNUSED34_Pin GPIO_PIN_9
#define UNUSED34_GPIO_Port GPIOC
#define UNUSED35_Pin GPIO_PIN_8
#define UNUSED35_GPIO_Port GPIOA
#define UNUSED36_Pin GPIO_PIN_10
#define UNUSED36_GPIO_Port GPIOA
#define UNUSED37_Pin GPIO_PIN_15
#define UNUSED37_GPIO_Port GPIOA
#define UNUSED38_Pin GPIO_PIN_10
#define UNUSED38_GPIO_Port GPIOC
#define UNUSED39_Pin GPIO_PIN_11
#define UNUSED39_GPIO_Port GPIOC
#define UNUSED40_Pin GPIO_PIN_12
#define UNUSED40_GPIO_Port GPIOC
#define UNUSED41_Pin GPIO_PIN_0
#define UNUSED41_GPIO_Port GPIOD
#define UNUSED42_Pin GPIO_PIN_1
#define UNUSED42_GPIO_Port GPIOD
#define EN_3V3A_Pin GPIO_PIN_2
#define EN_3V3A_GPIO_Port GPIOD
#define UNUSED43_Pin GPIO_PIN_3
#define UNUSED43_GPIO_Port GPIOD
#define UNUSED44_Pin GPIO_PIN_4
#define UNUSED44_GPIO_Port GPIOD
#define UNUSED45_Pin GPIO_PIN_5
#define UNUSED45_GPIO_Port GPIOD
#define UNUSED46_Pin GPIO_PIN_6
#define UNUSED46_GPIO_Port GPIOD
#define UNUSED47_Pin GPIO_PIN_7
#define UNUSED47_GPIO_Port GPIOD
#define UNUSED48_Pin GPIO_PIN_4
#define UNUSED48_GPIO_Port GPIOB
#define UNUSED49_Pin GPIO_PIN_5
#define UNUSED49_GPIO_Port GPIOB
#define UNUSED50_Pin GPIO_PIN_8
#define UNUSED50_GPIO_Port GPIOB
#define UNUSED51_Pin GPIO_PIN_9
#define UNUSED51_GPIO_Port GPIOB
#define UNUSED52_Pin GPIO_PIN_0
#define UNUSED52_GPIO_Port GPIOE
#define UNUSED53_Pin GPIO_PIN_1
#define UNUSED53_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
