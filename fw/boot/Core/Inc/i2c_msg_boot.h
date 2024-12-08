#pragma once
#include "i2c_msg_common.h"

#define I2C_BOOT_VERSION 1

#define I2C_REQ_BOOT_FLASH_UNLOCK (200)
#define I2C_REQ_BOOT_FLASH_LOCK (201)
#define I2C_REQ_BOOT_FLASH_ERASE (202)
#define I2C_REQ_BOOT_FLASH_WRITE (203)
#define I2C_REQ_BOOT_START_APP (204)
#define I2C_REQ_BOOT_GET_BOOT_VERSION (205)
#define I2C_REQ_BOOT_ENTER_DFU (206)

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
