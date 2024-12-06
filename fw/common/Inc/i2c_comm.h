#pragma once
#include "i2c_msg.h"

extern i2c_resp_all_t i2c_resp_buf;
uint8_t i2c_read_req(i2c_req_all_t *req);
