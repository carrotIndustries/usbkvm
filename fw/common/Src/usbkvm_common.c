#include "../Inc/usbkvm_common.h"
#include "../Inc/i2c_msg.h"
#include "main.h"

static uint8_t hw_model = I2C_MODEL_USBKVM;

uint8_t get_hw_model()
{
    return hw_model;
}

void update_hw_model()
{
    if(HAL_GPIO_ReadPin(HW_DETECT_GPIO_Port, HW_DETECT_Pin) == GPIO_PIN_RESET)
        hw_model = I2C_MODEL_USBKVM_PRO;
}
