/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_crs.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_cortex.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_pwr.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_gpio.h"

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
#define LED_USB_Pin GPIO_PIN_1
#define LED_USB_GPIO_Port GPIOA
#define LED_HID_Pin GPIO_PIN_2
#define LED_HID_GPIO_Port GPIOA
#define P5V_VGA_DETECT_Pin GPIO_PIN_2
#define P5V_VGA_DETECT_GPIO_Port GPIOB
#define INPUT_SEL2_Pin GPIO_PIN_8
#define INPUT_SEL2_GPIO_Port GPIOA
#define LED_VGA_Pin GPIO_PIN_9
#define LED_VGA_GPIO_Port GPIOA
#define LED_HDMI_Pin GPIO_PIN_10
#define LED_HDMI_GPIO_Port GPIOA
#define INPUT_SEL1_Pin GPIO_PIN_15
#define INPUT_SEL1_GPIO_Port GPIOA
#define VGA_SDA_Pin GPIO_PIN_3
#define VGA_SDA_GPIO_Port GPIOB
#define VGA_SCL_Pin GPIO_PIN_4
#define VGA_SCL_GPIO_Port GPIOB
#define HDMI_GPIO0_Pin GPIO_PIN_5
#define HDMI_GPIO0_GPIO_Port GPIOB
#define HDMI_GPIO1_Pin GPIO_PIN_6
#define HDMI_GPIO1_GPIO_Port GPIOB
#define EE_WP_Pin GPIO_PIN_7
#define EE_WP_GPIO_Port GPIOB
#define HW_DETECT_Pin GPIO_PIN_8
#define HW_DETECT_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
