#pragma once
#include "../../../common/Inc/i2c_msg_common.h"

#define I2C_APP_VERSION 5

#define I2C_REQ_NOP (0)
#define I2C_REQ_KEYBOARD_REPORT (1)
#define I2C_REQ_MOUSE_REPORT (2)
#define I2C_REQ_GET_INFO (3)
#define I2C_REQ_ENTER_BOOTLOADER (4)
#define I2C_REQ_GET_STATUS (5)
#define I2C_REQ_SET_LED (6)
#define I2C_REQ_FLASH_UNLOCK (7)
#define I2C_REQ_FLASH_LOCK (8)
#define I2C_REQ_FLASH_ERASE (9)
#define I2C_REQ_FLASH_WRITE (10)

typedef struct {
    I2C_REQ_COMMON
    uint8_t mod;
    uint8_t keycode[6];
} i2c_req_keyboard_report_t;

typedef struct {
    I2C_REQ_COMMON
    uint8_t button;
    uint8_t _pad;
    int16_t x;
    int16_t y;
    int8_t h;
    int8_t v;
} i2c_req_mouse_report_t;

#define I2C_LED_USB (1 << 0)
#define I2C_LED_HID (1 << 1)
#define I2C_LED_HDMI (1 << 2)

typedef struct {
  I2C_REQ_COMMON
  uint8_t mask;
  uint8_t stat;
} i2c_req_set_led_t;

typedef struct {
  I2C_REQ_COMMON;
  uint8_t first_page;
  uint8_t n_pages;
} i2c_req_flash_erase_t;

typedef struct {
  I2C_REQ_COMMON;
  uint16_t offset;
  uint8_t data[16];
} i2c_req_flash_write_t;

typedef union {
    i2c_req_unknown_t unk;
    i2c_req_keyboard_report_t kbd;
    i2c_req_mouse_report_t mouse;
    i2c_req_set_led_t set_led;
    i2c_req_flash_erase_t flash_erase;
    i2c_req_flash_write_t flash_write;
} i2c_req_app_all_t;

#define I2C_RESP_COMMON \
    uint8_t _head; \
    uint8_t seq; \

#define I2C_STATUS_VGA_CONNECTED (1<<0)

typedef struct {
    I2C_RESP_COMMON
    uint8_t status;
} i2c_resp_status_t;

typedef struct {
    I2C_RESP_COMMON
    uint8_t success;
} i2c_resp_flash_status_t;

typedef union {
    i2c_resp_unknown_t unk;
    i2c_resp_info_t info;
    i2c_resp_status_t status;
    i2c_resp_flash_status_t flash_status;
} i2c_resp_app_all_t;
