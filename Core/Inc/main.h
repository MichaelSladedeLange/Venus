/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//Ver 02.65 -  Fixed Uart buffer over flow and silent panic extern int vs char issue chaused hard fualt at siltent panic via sms.
#define FIRMWAREVERSION						"Venus Ver 02.65"

#define TRUE								1
#define FALSE								0

#define OUTPUT_ON_TIME						4

#define SIREN_SECONDS						60*4
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
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define GSM_ONOFF_Pin GPIO_PIN_0
#define GSM_ONOFF_GPIO_Port GPIOA
#define GSM_RI_Pin GPIO_PIN_1
#define GSM_RI_GPIO_Port GPIOA
#define GSM_TX_Pin GPIO_PIN_2
#define GSM_TX_GPIO_Port GPIOA
#define GSM_RX_Pin GPIO_PIN_3
#define GSM_RX_GPIO_Port GPIOA
#define GSM_PWR_Pin GPIO_PIN_4
#define GSM_PWR_GPIO_Port GPIOA
#define VOLTAGE_SENSE_Pin GPIO_PIN_0
#define VOLTAGE_SENSE_GPIO_Port GPIOB
#define MAINS_ONOFF_Pin GPIO_PIN_1
#define MAINS_ONOFF_GPIO_Port GPIOB
#define RELAY2_Pin GPIO_PIN_12
#define RELAY2_GPIO_Port GPIOB
#define RELAY1_Pin GPIO_PIN_13
#define RELAY1_GPIO_Port GPIOB
#define RFRX_LED_Pin GPIO_PIN_14
#define RFRX_LED_GPIO_Port GPIOB
#define RFRX_LEARN_BUTTON_Pin GPIO_PIN_15
#define RFRX_LEARN_BUTTON_GPIO_Port GPIOB
#define RFRX_LEARN_BUTTON_EXTI_IRQn EXTI4_15_IRQn
#define RFRX_IN_Pin GPIO_PIN_8
#define RFRX_IN_GPIO_Port GPIOA
#define RFRX_IN_EXTI_IRQn EXTI4_15_IRQn
#define OUT_ONOFF_Pin GPIO_PIN_15
#define OUT_ONOFF_GPIO_Port GPIOA
#define OUT_ALARM_Pin GPIO_PIN_3
#define OUT_ALARM_GPIO_Port GPIOB
#define OUT_PANIC_Pin GPIO_PIN_4
#define OUT_PANIC_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
