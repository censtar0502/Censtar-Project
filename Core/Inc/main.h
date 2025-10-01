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
#define D_C_Pin GPIO_PIN_1
#define D_C_GPIO_Port GPIOB
#define KeyCol1_Pin GPIO_PIN_7
#define KeyCol1_GPIO_Port GPIOE
#define KeyCol2_Pin GPIO_PIN_8
#define KeyCol2_GPIO_Port GPIOE
#define KeyCol3_Pin GPIO_PIN_9
#define KeyCol3_GPIO_Port GPIOE
#define KeyCol4_Pin GPIO_PIN_10
#define KeyCol4_GPIO_Port GPIOE
#define KeyCol5_Pin GPIO_PIN_11
#define KeyCol5_GPIO_Port GPIOE
#define KeyRow4_Pin GPIO_PIN_12
#define KeyRow4_GPIO_Port GPIOE
#define KeyRow3_Pin GPIO_PIN_13
#define KeyRow3_GPIO_Port GPIOE
#define KeyRow2_Pin GPIO_PIN_14
#define KeyRow2_GPIO_Port GPIOE
#define KeyRow1_Pin GPIO_PIN_15
#define KeyRow1_GPIO_Port GPIOE
#define CS_Pin GPIO_PIN_12
#define CS_GPIO_Port GPIOB
#define RST_Pin GPIO_PIN_14
#define RST_GPIO_Port GPIOB
#define EEPROM_SCL_Pin GPIO_PIN_6
#define EEPROM_SCL_GPIO_Port GPIOB
#define EEPROM_SDA_Pin GPIO_PIN_7
#define EEPROM_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
