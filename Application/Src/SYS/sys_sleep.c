/*
 * sys_sleep.c
 *
 *  Created on: Jul 27, 2025
 *      Author: ericv
 */

/* Private includes ----------------------------------------------------------*/

#include "main.h"
#include "sys_sleep.h"
#include "WS2812b-driver.h"
#include "PGA113-driver.h"
#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

#include <stdbool.h>

/* Private typedef -----------------------------------------------------------*/

typedef enum {
  SLEEP_NONE,
  SLEEP_LIGHT,
  SLEEP_DEEP
} SleepStates_t;

/* Private define ------------------------------------------------------------*/



/* Private macro -------------------------------------------------------------*/

#define CHECK_DISABLED_PERIPHERAL(x)    do { \
                                          if ((x) != HAL_OK) sleep_error = true; \
                                        } while (0)

/* Private variables ---------------------------------------------------------*/

osEventFlagsId_t sleep_events = NULL;
static volatile bool sleep_error = false;

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;

extern CORDIC_HandleTypeDef hcordic;

extern DAC_HandleTypeDef hdac1;
extern DMA_HandleTypeDef hdma_dac1_ch2;
extern DMA_HandleTypeDef hdma_dac1_ch1;

extern DTS_HandleTypeDef hdts;

extern I2C_HandleTypeDef hi2c1;
extern DMA_HandleTypeDef hdma_i2c1_rx;
extern DMA_HandleTypeDef hdma_i2c1_tx;

extern SPI_HandleTypeDef hspi4;
extern DMA_HandleTypeDef hdma_spi4_tx;
extern DMA_HandleTypeDef hdma_spi4_rx;

extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim8;
extern TIM_HandleTypeDef htim16;
extern TIM_HandleTypeDef htim17;
extern DMA_HandleTypeDef hdma_tim3_ch1;

extern UART_HandleTypeDef huart5;
extern DMA_HandleTypeDef hdma_uart5_rx;
extern DMA_HandleTypeDef hdma_uart5_tx;

extern PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* Private function prototypes -----------------------------------------------*/

bool createSleepEvents();
void handleSleep();
SleepStates_t getSleepMode();
void enterDeepSleep();
void disablePeripherals();
void disableInterrupts();

/* Exported function definitions ---------------------------------------------*/

bool Sleep_Init()
{
  return createSleepEvents();
}

void Sleep_Check()
{
  handleSleep();
}

/* Private function definitions ----------------------------------------------*/

bool createSleepEvents()
{
  sleep_events = osEventFlagsNew(NULL);

  return sleep_events != NULL;
}

void handleSleep()
{
  SleepStates_t sleep_state = getSleepMode();

  switch (sleep_state) {
    case SLEEP_DEEP:
      enterDeepSleep();
      break;
    case SLEEP_LIGHT:
      break;
    case SLEEP_NONE:
    default:
  }
}

SleepStates_t getSleepMode()
{
  if (sleep_events == NULL) {
    return SLEEP_NONE;
  }

  uint32_t events = osEventFlagsGet(sleep_events);

  if (events & SLEEP_REQUEST_LIGHT) {
    return SLEEP_LIGHT;
  }

  if (events & SLEEP_REQUEST_DEEP) {
    return SLEEP_DEEP;
  }

  return SLEEP_NONE;
}

// TODO: make it safe by ensuring that there are no pending saves etc
void enterDeepSleep()
{
  vTaskSuspendAll();

//  SCB_CleanInvalidateDCache();
//  SCB_DisableDCache();

  WS_Update(0);

  PGA_Shutdown();

  HAL_Delay(5);

  disablePeripherals();

  HAL_SuspendTick();
  HAL_PWREx_ControlStopModeVoltageScaling(PWR_REGULATOR_SVOS_SCALE5);
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

  reinitializePeripherals();

//  SCB_EnableDCache();

  HAL_ResumeTick();

  osEventFlagsClear(sleep_events, SLEEP_REQUEST_DEEP);
  osEventFlagsSet(sleep_events, SLEEP_WAKEUP_MESS);

  xTaskResumeAll();
}

void disablePeripherals()
{
  HAL_ADCEx_EnterADCDeepPowerDownMode(&hadc1);
  HAL_ADCEx_EnterADCDeepPowerDownMode(&hadc2);
  HAL_ADC_Stop_DMA(&hadc1);
  HAL_ADC_Stop_DMA(&hadc2);
  HAL_ADC_Stop(&hadc3);
  __HAL_RCC_ADC12_CLK_DISABLE();
  __HAL_RCC_ADC3_CLK_DISABLE();
  
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
  HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_2);
  __HAL_RCC_DAC12_CLK_DISABLE();
  
  HAL_DTS_Stop(&hdts);
  __HAL_RCC_DTS_CLK_DISABLE();
  
  HAL_I2C_MspDeInit(&hi2c1);
  __HAL_RCC_I2C1_CLK_DISABLE();
  
  HAL_SPI_MspDeInit(&hspi4);
  __HAL_RCC_SPI4_CLK_DISABLE();
  
  HAL_TIM_Base_Stop(&htim3);
  HAL_TIM_Base_Stop(&htim6);
  HAL_TIM_Base_Stop(&htim8);
  HAL_TIM_Base_Stop(&htim16);
  HAL_TIM_Base_Stop(&htim17);
  __HAL_RCC_TIM3_CLK_DISABLE();
  __HAL_RCC_TIM6_CLK_DISABLE();
  __HAL_RCC_TIM8_CLK_DISABLE();
  __HAL_RCC_TIM16_CLK_DISABLE();
  __HAL_RCC_TIM17_CLK_DISABLE();
  
  HAL_UART_MspDeInit(&huart5);
  __HAL_RCC_UART5_CLK_DISABLE();
  
  __HAL_RCC_CORDIC_CLK_DISABLE();
  
  HAL_PCD_Stop(&hpcd_USB_OTG_HS);
  __HAL_RCC_USB1_OTG_HS_CLK_DISABLE();
  
  __HAL_RCC_DMA1_CLK_DISABLE();
  __HAL_RCC_DMA2_CLK_DISABLE();
  __HAL_RCC_BDMA_CLK_DISABLE();

  __HAL_RCC_GPIOE_CLK_DISABLE();  // Has many unused pins
  __HAL_RCC_GPIOA_CLK_DISABLE();  // Check if needed for wakeup
  __HAL_RCC_GPIOB_CLK_DISABLE();  // Has many unused pins
  // Keep GPIOC enabled (has some control pins)
  // Keep GPIOD enabled (has control pins and potential wakeup)
  __HAL_RCC_GPIOH_CLK_DISABLE();  // Usually just oscillator pins
}

void disableInterrupts()
{
  HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream2_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream3_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream4_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream6_IRQn);
  HAL_NVIC_DisableIRQ(DMA1_Stream7_IRQn);
  HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn);
  HAL_NVIC_DisableIRQ(DMA2_Stream1_IRQn);
  HAL_NVIC_DisableIRQ(DMA2_Stream2_IRQn);
  HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
  
  // Disable peripheral interrupts
  HAL_NVIC_DisableIRQ(ADC_IRQn);
  HAL_NVIC_DisableIRQ(TIM6_DAC_IRQn);
  HAL_NVIC_DisableIRQ(TIM8_UP_TIM13_IRQn);
  HAL_NVIC_DisableIRQ(TIM1_UP_TIM10_IRQn);
  HAL_NVIC_DisableIRQ(TIM1_TRG_COM_TIM11_IRQn);
  HAL_NVIC_DisableIRQ(UART5_IRQn);
  HAL_NVIC_DisableIRQ(I2C1_EV_IRQn);
  HAL_NVIC_DisableIRQ(I2C1_ER_IRQn);
  HAL_NVIC_DisableIRQ(SPI4_IRQn);
  HAL_NVIC_DisableIRQ(OTG_HS_IRQn);
}
