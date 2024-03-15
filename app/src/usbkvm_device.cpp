#include "usbkvm_device.hpp"
#include "mshal.hpp"
#include "i2c_util.hpp"
#include "usbkvm_mcu.hpp"

static const uint8_t s_mcu_addr = 0x0a;

UsbKvmDevice::UsbKvmDevice(const std::string &name)
{
    m_hal = std::make_unique<MsHal>(name);
    m_i2c1dev_mcu = std::make_unique<I2COneDevice>(*m_hal, s_mcu_addr);
    m_mcu = std::make_unique<UsbKvmMcu>(*m_i2c1dev_mcu);
}

void UsbKvmDevice::enter_bootloader()
{
    if (!m_mcu)
        m_mcu = std::make_unique<UsbKvmMcu>(*m_i2c1dev_mcu);
    m_mcu->enter_bootloader();
    m_mcu.reset();
}

void UsbKvmDevice::delete_mcu()
{
    m_mcu.reset();
}

UsbKvmDevice::~UsbKvmDevice() = default;
