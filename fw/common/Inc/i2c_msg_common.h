#pragma once
#include <stdint.h>
#ifdef USBKVM_FIRMWARE
#include "i2c_req_enum_fw.h"
#else
#include "i2c_req_enum_app.hpp"
#endif

#define I2C_REQ_COMMON \
    uint8_t type;\
    uint8_t seq;

REQ_ENUM_BEGIN(i2c_req_common)
    REQ_ENUM_ITEM(I2C_REQ_COM, NOP, 0)
    REQ_ENUM_ITEM(I2C_REQ_COM, GET_INFO, 3)
REQ_ENUM_END
    
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

DEFINE_XFER(i2c_req_common_t::GET_INFO, i2c_req_unknown_t, i2c_resp_info_t)
