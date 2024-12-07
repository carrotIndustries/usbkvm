#pragma once

#ifdef USBKVM_BOOT
#include "i2c_msg_boot.h"
typedef i2c_resp_boot_all_t i2c_resp_all_t;
typedef i2c_req_boot_all_t i2c_req_all_t;

#else
#include "i2c_msg_app.h"
typedef i2c_resp_app_all_t i2c_resp_all_t;
typedef i2c_req_app_all_t i2c_req_all_t;
#endif

extern i2c_resp_all_t i2c_resp_buf;
uint8_t i2c_read_req(i2c_req_all_t *req);

