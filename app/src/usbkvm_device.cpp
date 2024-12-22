#include "usbkvm_device.hpp"
#include "mshal.hpp"
#include "i2c_util.hpp"
#include "usbkvm_mcu.hpp"

namespace usbkvm {

static const uint8_t s_mcu_addr = 0x0a;

UsbKvmDevice::UsbKvmDevice(const std::string &path)
{
    m_hal = std::make_unique<MsHal>(path);
    m_i2c1dev_mcu = std::make_unique<I2COneDevice>(*m_hal, s_mcu_addr);
    m_mcu = std::make_unique<UsbKvmMcu>(*m_i2c1dev_mcu);
}

void UsbKvmDevice::enter_bootloader()
{
    if (!m_mcu)
        m_mcu = std::make_unique<UsbKvmMcu>(*m_i2c1dev_mcu);
    m_mcu->enter_bootloader();
    set_boot_mcu();
}

void UsbKvmDevice::delete_mcu()
{
    m_mcu.reset();
}

void UsbKvmDevice::set_boot_mcu()
{
    m_mcu_boot = std::move(m_mcu);
    m_mcu.reset();
}

void UsbKvmDevice::leave_bootloader()
{
    if (!m_mcu_boot)
        return;
    m_mcu = std::move(m_mcu_boot);
    m_mcu_boot.reset();
}

void UsbKvmDevice::set_model(Model model)
{
    if (m_model.has_value())
        throw std::runtime_error("Model already set");
    m_model = model;
}

void UsbKvmDevice::close_hal()
{
    m_hal->close();
}

UsbKvmDevice::~UsbKvmDevice() = default;

} // namespace usbkvm
