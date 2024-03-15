#pragma once
#include <stdint.h>

#define I2C_VERSION 1

#define I2C_REQ_COMMON \
    uint8_t type;\
    uint8_t seq;

#define I2C_REQ_NOP (0)
#define I2C_REQ_KEYBOARD_REPORT (1)
#define I2C_REQ_MOUSE_REPORT (2)
#define I2C_REQ_GET_VERSION (3)
#define I2C_REQ_ENTER_BOOTLOADER (4)
    
typedef struct {
    I2C_REQ_COMMON
} i2c_req_unknown_t;

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

typedef union {
    i2c_req_unknown_t unk;
    i2c_req_keyboard_report_t kbd;
    i2c_req_mouse_report_t mouse;
} i2c_req_all_t;

#define I2C_RESP_COMMON \
    uint8_t _head; \
    uint8_t seq; \

typedef struct {
    I2C_RESP_COMMON
} i2c_resp_unknown_t;

typedef struct {
    I2C_RESP_COMMON
    uint8_t version;
} i2c_resp_version_t;

typedef union {
    i2c_resp_unknown_t unk;
    i2c_resp_version_t version;
} i2c_resp_all_t;
