#pragma once
#include <stdint.h>

#define I2C_REQ_COMMON \
    uint8_t type;\
    uint8_t seq;

#define I2C_REQ_COM_NOP (0)
#define I2C_REQ_COM_GET_INFO (3)

typedef struct {
    I2C_REQ_COMMON
} i2c_req_unknown_t;

#define I2C_RESP_COMMON \
    uint8_t _head; \
    uint8_t seq; \

typedef struct {
    I2C_RESP_COMMON
} i2c_resp_unknown_t;

#define I2C_MODEL_USBKVM (0)
#define I2C_MODEL_USBKVM_PRO (1)
#define I2C_VERSION_MASK (0x7f)
#define I2C_VERSION_BOOT (0x80)

typedef struct {
    I2C_RESP_COMMON
    uint8_t version;
    uint8_t model;
} i2c_resp_info_t;
