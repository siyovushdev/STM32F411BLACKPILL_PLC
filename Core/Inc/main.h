/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DO0_Pin GPIO_PIN_13
#define DO0_GPIO_Port GPIOC
#define DO1_Pin GPIO_PIN_14
#define DO1_GPIO_Port GPIOC
#define DO2_Pin GPIO_PIN_15
#define DO2_GPIO_Port GPIOC
#define DO3_Pin GPIO_PIN_4
#define DO3_GPIO_Port GPIOA
#define DAC_SYNC_Pin GPIO_PIN_0
#define DAC_SYNC_GPIO_Port GPIOB
#define ADC_RST_Pin GPIO_PIN_1
#define ADC_RST_GPIO_Port GPIOB
#define DO4_Pin GPIO_PIN_2
#define DO4_GPIO_Port GPIOB
#define DI0_Pin GPIO_PIN_10
#define DI0_GPIO_Port GPIOB
#define ADC_CS_Pin GPIO_PIN_12
#define ADC_CS_GPIO_Port GPIOB
#define DAC_CLR_Pin GPIO_PIN_13
#define DAC_CLR_GPIO_Port GPIOB
#define DAC_LDAC_Pin GPIO_PIN_14
#define DAC_LDAC_GPIO_Port GPIOB
#define DI1_Pin GPIO_PIN_15
#define DI1_GPIO_Port GPIOB
#define DI2_Pin GPIO_PIN_11
#define DI2_GPIO_Port GPIOA
#define DI3_Pin GPIO_PIN_12
#define DI3_GPIO_Port GPIOA
#define ADC_BUSY_Pin GPIO_PIN_15
#define ADC_BUSY_GPIO_Port GPIOA
#define ADC_BUSY_EXTI_IRQn EXTI15_10_IRQn
#define ADC_CONVST_Pin GPIO_PIN_5
#define ADC_CONVST_GPIO_Port GPIOB
#define DI4_Pin GPIO_PIN_8
#define DI4_GPIO_Port GPIOB
#define DI5_Pin GPIO_PIN_9
#define DI5_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
