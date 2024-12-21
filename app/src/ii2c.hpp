#pragma once
#include <stdint.h>
#include <span>

namespace usbkvm {

class II2C {
public:
    virtual void i2c_transfer(uint8_t device_addr, std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd) = 0;
};

class II2COneDevice {
public:
    virtual void i2c_transfer(std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd) = 0;
};

} // namespace usbkvm
