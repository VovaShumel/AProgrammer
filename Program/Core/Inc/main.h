/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32f1xx_hal.h"

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
#define BLUE_LED_Pin GPIO_PIN_13
#define BLUE_LED_GPIO_Port GPIOC
#define ROTATE_STEP_Pin GPIO_PIN_14
#define ROTATE_STEP_GPIO_Port GPIOC
#define ROTATE_DIR_Pin GPIO_PIN_15
#define ROTATE_DIR_GPIO_Port GPIOC
#define P24V_OK_Pin GPIO_PIN_0
#define P24V_OK_GPIO_Port GPIOA
#define VALVE_ON_Pin GPIO_PIN_3
#define VALVE_ON_GPIO_Port GPIOA
#define PUMP_DIR_Pin GPIO_PIN_4
#define PUMP_DIR_GPIO_Port GPIOA
#define PUMP_STEP_Pin GPIO_PIN_5
#define PUMP_STEP_GPIO_Port GPIOA
#define PUMP_ALM_Pin GPIO_PIN_6
#define PUMP_ALM_GPIO_Port GPIOA
#define PUMP_ENA_Pin GPIO_PIN_7
#define PUMP_ENA_GPIO_Port GPIOA
#define KEY_DOWN_Pin GPIO_PIN_0
#define KEY_DOWN_GPIO_Port GPIOB
#define KEY_ESC_Pin GPIO_PIN_1
#define KEY_ESC_GPIO_Port GPIOB
#define KEY_ENTER_Pin GPIO_PIN_10
#define KEY_ENTER_GPIO_Port GPIOB
#define KEY_UP_Pin GPIO_PIN_11
#define KEY_UP_GPIO_Port GPIOB
#define HOME2_Pin GPIO_PIN_12
#define HOME2_GPIO_Port GPIOB
#define PEDAL_Pin GPIO_PIN_13
#define PEDAL_GPIO_Port GPIOB
#define HOME1_Pin GPIO_PIN_14
#define HOME1_GPIO_Port GPIOB
#define VALVE_STATE_Pin GPIO_PIN_15
#define VALVE_STATE_GPIO_Port GPIOB
#define CONSOLE_TX_Pin GPIO_PIN_6
#define CONSOLE_TX_GPIO_Port GPIOB
#define CONSOLE_RX_Pin GPIO_PIN_7
#define CONSOLE_RX_GPIO_Port GPIOB
#define LCD_SCL_Pin GPIO_PIN_8
#define LCD_SCL_GPIO_Port GPIOB
#define LCD_SDA_Pin GPIO_PIN_9
#define LCD_SDA_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
