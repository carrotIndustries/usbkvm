#include "i2c_util.hpp"

I2COneDevice::I2COneDevice(II2C &i2c, uint8_t device_addr) : m_i2c(i2c), m_device_addr(device_addr)
{
}

void I2COneDevice::i2c_transfer(std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd)
{
    m_i2c.i2c_transfer(m_device_addr, data_wr, data_rd);
}

I2COneDevice::~I2COneDevice() = default;
