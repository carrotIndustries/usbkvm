#pragma once
#include "i2c_msg_common.h"

#define I2C_BOOT_VERSION 1

REQ_ENUM_BEGIN(i2c_req_boot)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, FLASH_UNLOCK     , 200)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, FLASH_LOCK       , 201)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, FLASH_ERASE      , 202)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, FLASH_WRITE      , 203)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, START_APP        , 204)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, GET_BOOT_VERSION , 205)
  REQ_ENUM_ITEM(I2C_REQ_BOOT, ENTER_DFU        , 206)
REQ_ENUM_END

typedef struct {
  I2C_REQ_COMMON;
  uint8_t first_page;
  uint8_t n_pages;
} i2c_req_boot_flash_erase_t;

typedef struct {
  I2C_REQ_COMMON;
  uint16_t offset;
  uint8_t data[256];
} i2c_req_boot_flash_write_t;

typedef union {
    i2c_req_unknown_t unk;
    i2c_req_boot_flash_erase_t flash_erase;
    i2c_req_boot_flash_write_t flash_write;
} i2c_req_boot_all_t;

#define I2C_VERSION_BOOT_APP_INVALID_MAGIC (0x7d)
#define I2C_VERSION_BOOT_APP_HEADER_CRC_MISMATCH (0x7e)
#define I2C_VERSION_BOOT_APP_CRC_MISMATCH (0x7f)
#define I2C_VERSION_BOOT_APP_OK (0)

typedef struct {
    I2C_RESP_COMMON
    uint8_t success;
} i2c_resp_boot_flash_status_t;

typedef struct {
    I2C_RESP_COMMON
    uint8_t version;
} i2c_resp_boot_boot_version_t;

typedef union {
    i2c_resp_unknown_t unk;
    i2c_resp_info_t info;
    i2c_resp_boot_flash_status_t flash_status;
    i2c_resp_boot_boot_version_t boot_version;
} i2c_resp_boot_all_t;

DEFINE_XFER(i2c_req_boot_t::FLASH_UNLOCK     , i2c_req_unknown_t          , i2c_resp_boot_flash_status_t )
DEFINE_XFER(i2c_req_boot_t::FLASH_LOCK       , i2c_req_unknown_t          , void                         )
DEFINE_XFER(i2c_req_boot_t::FLASH_ERASE      , i2c_req_boot_flash_erase_t , i2c_resp_boot_flash_status_t )
DEFINE_XFER(i2c_req_boot_t::FLASH_WRITE      , i2c_req_boot_flash_write_t , i2c_resp_boot_flash_status_t )
DEFINE_XFER(i2c_req_boot_t::START_APP        , i2c_req_unknown_t          , void                         )
DEFINE_XFER(i2c_req_boot_t::GET_BOOT_VERSION , i2c_req_unknown_t          , i2c_resp_boot_boot_version_t )
DEFINE_XFER(i2c_req_boot_t::ENTER_DFU        , i2c_req_unknown_t          , void                         )
