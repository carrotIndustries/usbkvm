/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "../../../common/Inc/i2c_comm.h"
#include "../../../common/Inc/usbkvm_common.h"
#include "../../../common/Inc/flash_header.h"
#include "dfu.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


static const volatile flash_header_t *g_flash_header = (void*)(0x8000000+0x2000);

static uint8_t validate_app()
{
  if(g_flash_header->magic != FLASH_HEADER_MAGIC)
    return I2C_VERSION_BOOT_APP_INVALID_MAGIC;
  if(g_flash_header->header_crc != HAL_CRC_Calculate(&hcrc, (void*)g_flash_header, sizeof(flash_header_t)-4))
    return I2C_VERSION_BOOT_APP_HEADER_CRC_MISMATCH;
  if(g_flash_header->app_crc != HAL_CRC_Calculate(&hcrc, (void*)0x8000000+0x2000+0x20, g_flash_header->app_size))
    return I2C_VERSION_BOOT_APP_CRC_MISMATCH;
  return I2C_VERSION_BOOT_APP_OK;
}

void i2c_req_handle_get_info(const i2c_req_unknown_t *unk)
{
  uint8_t app_valid = validate_app();
  if(app_valid == I2C_VERSION_BOOT_APP_OK)
    i2c_resp_buf.info.version = g_flash_header->app_version | I2C_VERSION_BOOT;
  else
    i2c_resp_buf.info.version = app_valid | I2C_VERSION_BOOT;
  i2c_resp_buf.info.model = get_hw_model();
  i2c_resp_buf.info.seq = unk->seq;
}

void i2c_req_handle_flash_unlock(const i2c_req_unknown_t *unk)
{
  HAL_StatusTypeDef status = HAL_FLASH_Unlock();
  i2c_resp_buf.flash_status.success = (status == HAL_OK);
  i2c_resp_buf.flash_status.seq = unk->seq;
}

void i2c_req_handle_flash_erase(const i2c_req_boot_flash_erase_t *req)
{
  uint32_t PageError = 0;
  /* Variable contains Flash operation status */
  HAL_StatusTypeDef status;
  FLASH_EraseInitTypeDef eraseinitstruct;

  /* Clear OPTVERR bit set on virgin samples */

  eraseinitstruct.TypeErase = FLASH_TYPEERASE_PAGES;
  eraseinitstruct.PageAddress = req->first_page * FLASH_PAGE_SIZE;
  eraseinitstruct.NbPages = req->n_pages;
  status = HAL_FLASHEx_Erase(&eraseinitstruct, &PageError);
  
  i2c_resp_buf.flash_status.success = (status == HAL_OK);
  i2c_resp_buf.flash_status.seq = req->seq;
}

void i2c_req_handle_flash_write(const i2c_req_boot_flash_write_t *req)
{
  HAL_StatusTypeDef status = HAL_OK;
  for(uint16_t offset = 0; offset < sizeof(req->data); offset+=8) {
    uint64_t dat;
    memcpy(&dat, req->data + offset, 8);
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, req->offset + offset, dat);
    if(status != HAL_OK)
      break;
  }
  
  i2c_resp_buf.flash_status.success = (status == HAL_OK);
  i2c_resp_buf.flash_status.seq = req->seq;
}

void i2c_req_handle_get_boot_version(const i2c_req_unknown_t *unk)
{
  i2c_resp_buf.boot_version.version = I2C_BOOT_VER1SION;
  i2c_resp_buf.flash_status.seq = unk->seq;
}

typedef  void (*pFunction)(void);
pFunction JumpToApplication;
uint32_t JumpAddress;

static void start_app() {
  SysTick->CTRL = 0;
  HAL_NVIC_DisableIRQ(SysTick_IRQn);
  HAL_NVIC_DisableIRQ(I2C1_IRQn);

  /* Jump to user application */
  JumpAddress = *(__IO uint32_t*) (0x8002020 + 4);
  JumpToApplication = (pFunction) JumpAddress;

  /* Initialize user application's Stack Pointer */
  __set_MSP(*(__IO uint32_t*) 0x8002020);
  JumpToApplication();
  while(1)
    ;
}

static void i2c_req_dispatch(i2c_req_all_t *req)
{
  HAL_GPIO_TogglePin(LED_USB_GPIO_Port, LED_USB_Pin);
  switch(req->unk.type) {
    case I2C_REQ_GET_INFO:
      i2c_req_handle_get_info(&req->unk);
      break;
    case I2C_REQ_BOOT_FLASH_UNLOCK:
      i2c_req_handle_flash_unlock(&req->unk);
      break;
    case I2C_REQ_BOOT_FLASH_LOCK:
      HAL_FLASH_Lock();
      break;
    case I2C_REQ_BOOT_FLASH_ERASE:
      i2c_req_handle_flash_erase(&req->flash_erase);
      break;
    case I2C_REQ_BOOT_FLASH_WRITE:
      i2c_req_handle_flash_write(&req->flash_write);
      break;
    case I2C_REQ_BOOT_START_APP:
      start_app();
      break;
    case I2C_REQ_BOOT_GET_BOOT_VERSION:
      i2c_req_handle_get_boot_version(&req->unk);
      break;
    case I2C_REQ_BOOT_ENTER_DFU:
      enter_dfu();
      break;
    default:;
  }
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  jump_to_dfu();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */
    
  update_hw_model();
  
  hi2c1.Instance->TXDR = 0x83;
  
  HAL_I2C_EnableListen_IT(&hi2c1);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    i2c_req_all_t req;
    if(i2c_read_req(&req)) {
      i2c_req_dispatch(&req);
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1)
  {
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_HSI48_Enable();

   /* Wait till HSI48 is ready */
  while(LL_RCC_HSI48_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI48);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI48)
  {

  }
  LL_SetSystemCoreClock(48000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
  LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_HSI);
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x2000090E;
  hi2c1.Init.OwnAddress1 = 20;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_ENABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_USB_Pin|LED_HID_Pin|INPUT_SEL2_Pin|LED_VGA_Pin
                          |LED_HDMI_Pin|INPUT_SEL1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_USB_Pin LED_HID_Pin INPUT_SEL2_Pin LED_VGA_Pin
                           LED_HDMI_Pin INPUT_SEL1_Pin */
  GPIO_InitStruct.Pin = LED_USB_Pin|LED_HID_Pin|INPUT_SEL2_Pin|LED_VGA_Pin
                          |LED_HDMI_Pin|INPUT_SEL1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : P5V_VGA_DETECT_Pin HW_DETECT_Pin */
  GPIO_InitStruct.Pin = P5V_VGA_DETECT_Pin|HW_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : VGA_SDA_Pin VGA_SCL_Pin HDMI_GPIO0_Pin HDMI_GPIO1_Pin
                           EE_WP_Pin */
  GPIO_InitStruct.Pin = VGA_SDA_Pin|VGA_SCL_Pin|HDMI_GPIO0_Pin|HDMI_GPIO1_Pin
                          |EE_WP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
