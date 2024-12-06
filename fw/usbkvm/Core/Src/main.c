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
#include "tusb.h"
#include "usb_descriptors.h"
#include "i2c_msg.h"
#include "common/tusb_fifo.h"
#include "boot.h"
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
I2C_HandleTypeDef hi2c1;

PCD_HandleTypeDef hpcd_USB_FS;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USB_PCD_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static uint8_t hw_model = I2C_MODEL_USBKVM;

static uint8_t read_vga_connected()
{
  return !HAL_GPIO_ReadPin(P5V_VGA_DETECT_GPIO_Port, P5V_VGA_DETECT_Pin);
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {}

void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
  (void)instance;
  (void)protocol;

  // nothing to do since we use the same compatible boot report for both Boot
  // and Report mode.
  // TODO set a indicator for user
}

typedef struct TU_ATTR_PACKED {
  uint8_t buttons;  /**< buttons mask for currently pressed buttons in the mouse. */
  int16_t x;    /**< Current delta x movement of the mouse. */
  int16_t y;    /**< Current delta y movement on the mouse. */
  int8_t wheel; /**< Current delta wheel movement on the mouse. */
  int8_t pan;   // using AC Pan
} hid_mouse_abs_report_t;


static void *ptxbuf ;
static void *prxbuf;

static i2c_req_all_t i2c_req_rxbuf;
static i2c_resp_all_t i2c_resp_txbuf;
static i2c_resp_all_t i2c_resp_buf;
static uint8_t is_rx;

static void copy_resp(i2c_resp_all_t *dest, const i2c_resp_all_t *src)
{
  const uint8_t *ps = (const void*)src;
  uint8_t *pd = (void*)dest;
  ps+=sizeof(i2c_resp_all_t);
  pd+=sizeof(i2c_resp_all_t);
  do {
    ps--;
    pd--;
    *pd = *ps;
  } while(ps != (void*)src);
}


TU_FIFO_DEF(i2c_req_fifo,  4, i2c_req_all_t, 0);

void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{
  if( TransferDirection==I2C_DIRECTION_TRANSMIT ) {
    is_rx = 1;
    prxbuf = &i2c_req_rxbuf;
    HAL_I2C_Slave_Seq_Receive_IT(hi2c, prxbuf, 1, I2C_NEXT_FRAME);
    prxbuf++;
  }
  else {
    is_rx = 0;
    copy_resp(&i2c_resp_txbuf, &i2c_resp_buf);
    ptxbuf = &i2c_resp_txbuf;
    ptxbuf++; // skip _head
    HAL_I2C_Slave_Seq_Transmit_IT(hi2c, ptxbuf, 1, I2C_NEXT_FRAME);
    ptxbuf++;
  }
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_Slave_Seq_Transmit_IT(hi2c, ptxbuf, 1, I2C_NEXT_FRAME);
  ptxbuf++;
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c) {
  HAL_I2C_Slave_Seq_Receive_IT(hi2c, prxbuf, 1, I2C_NEXT_FRAME);
  prxbuf++;
}

void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if(is_rx) {
    tu_fifo_write(&i2c_req_fifo, &i2c_req_rxbuf);
  }
  hi2c1.Instance->TXDR = 0x83;
  HAL_I2C_EnableListen_IT(hi2c); // slave is ready again
}

void i2c_req_handle_keyboard_report(i2c_req_keyboard_report_t *req)
{
  if (!tud_hid_n_ready(ITF_NUM_KEYBOARD))
    return;

  tud_hid_n_keyboard_report(ITF_NUM_KEYBOARD, 0, req->mod, req->keycode);
}

void i2c_req_handle_mouse_report(i2c_req_mouse_report_t *req)
{
  if (!tud_hid_n_ready(ITF_NUM_MOUSE))
    return;
  
  hid_mouse_abs_report_t report = {.buttons = req->button,
                                     .x = req->x,
                                     .y = req->y,
                                     .wheel = req->v,
                                     .pan = req->h};

  tud_hid_n_report(ITF_NUM_MOUSE, 0, &report, sizeof(report));
}

void i2c_req_handle_get_info(const i2c_req_unknown_t *unk)
{
  i2c_resp_buf.info.version = I2C_VERSION;
  i2c_resp_buf.info.model = hw_model;
  i2c_resp_buf.info.seq = unk->seq;
}

void i2c_req_handle_get_status(const i2c_req_unknown_t *unk)
{
  uint8_t status = 0;
  if(read_vga_connected())
    status |= I2C_STATUS_VGA_CONNECTED;
  i2c_resp_buf.status.status = status;
  i2c_resp_buf.status.seq = unk->seq;
}

static i2c_req_set_led_t led_override;

static uint8_t get_led(uint8_t led, uint8_t orig_state) {
  if (led_override.mask & led)
    return led_override.stat & led;
  return orig_state;
}

void i2c_req_handle_set_led(const i2c_req_set_led_t *led) {
  led_override = *led;
}

static uint16_t led_usb_counter = 0;

void update_leds()
{
  HAL_GPIO_WritePin(LED_USB_GPIO_Port, LED_USB_Pin,
                    get_led(I2C_LED_USB, led_usb_counter != 0));
  if (led_usb_counter)
    led_usb_counter--;
}

void i2c_req_dispatch(i2c_req_all_t *req)
{
  switch(req->unk.type) {
    case I2C_REQ_KEYBOARD_REPORT:
      led_usb_counter = 50;
      i2c_req_handle_keyboard_report(&req->kbd);
      break;
    case I2C_REQ_MOUSE_REPORT:
      led_usb_counter = 50;
      i2c_req_handle_mouse_report(&req->mouse);
      break;
    case I2C_REQ_GET_INFO:
      i2c_req_handle_get_info(&req->unk);
      break;
    case I2C_REQ_GET_STATUS:
      i2c_req_handle_get_status(&req->unk);
      break;
    case I2C_REQ_SET_LED:
      i2c_req_handle_set_led(&req->set_led);
      break;
    case I2C_REQ_ENTER_BOOTLOADER:
      enter_bootloader();
      break;
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
  jump_to_bootloader();
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
  MX_USB_PCD_Init();
  /* USER CODE BEGIN 2 */
  
  if(HAL_GPIO_ReadPin(HW_DETECT_GPIO_Port, HW_DETECT_Pin) == GPIO_PIN_RESET)
    hw_model = I2C_MODEL_USBKVM_PRO;
  
  if(hw_model == I2C_MODEL_USBKVM_PRO) {
    HAL_GPIO_WritePin(INPUT_SEL1_GPIO_Port, INPUT_SEL1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(INPUT_SEL2_GPIO_Port, INPUT_SEL2_Pin, GPIO_PIN_RESET);
  }

  tusb_init();
  hi2c1.Instance->TXDR = 0x83;
  
  HAL_I2C_EnableListen_IT(&hi2c1);
  led_usb_counter = 1000;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    tud_task();
    HAL_GPIO_WritePin(LED_HID_GPIO_Port, LED_HID_Pin,
                      get_led(I2C_LED_HID, tud_hid_n_ready(ITF_NUM_KEYBOARD)));
    HAL_GPIO_WritePin(LED_HDMI_GPIO_Port, LED_HDMI_Pin,
                      get_led(I2C_LED_HDMI, 0));

    if (hw_model == I2C_MODEL_USBKVM_PRO) {
      uint8_t has_vga = read_vga_connected();
      HAL_GPIO_WritePin(LED_VGA_GPIO_Port, LED_VGA_Pin, has_vga);
      HAL_GPIO_WritePin(INPUT_SEL2_GPIO_Port, INPUT_SEL2_Pin, !has_vga);
    }
    
    i2c_req_all_t req;
    if(tu_fifo_read(&i2c_req_fifo, &req)) {
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI48;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_HSI;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Enable the SYSCFG APB clock
  */
  __HAL_RCC_CRS_CLK_ENABLE();
  /** Configures CRS
  */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_USB;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,1000);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
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
  * @brief USB Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_PCD_Init(void)
{

  /* USER CODE BEGIN USB_Init 0 */

  /* USER CODE END USB_Init 0 */

  /* USER CODE BEGIN USB_Init 1 */

  /* USER CODE END USB_Init 1 */
  hpcd_USB_FS.Instance = USB;
  hpcd_USB_FS.Init.dev_endpoints = 8;
  hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_FS.Init.battery_charging_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_Init 2 */

  /* USER CODE END USB_Init 2 */

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
  while (1) {
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
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n", file,
     line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
