#pragma once
#include "ii2c.hpp"

namespace usbkvm {

class I2COneDevice : public II2COneDevice {
public:
    I2COneDevice(II2C &i2c, uint8_t device_addr);
    void i2c_transfer(std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd) override;

    virtual ~I2COneDevice();

private:
    II2C &m_i2c;
    uint8_t m_device_addr;
};

} // namespace usbkvm
