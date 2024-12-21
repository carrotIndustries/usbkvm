#pragma once
#include <memory>
#include <optional>
#include "model.hpp"

namespace usbkvm {

class MsHal;
class I2COneDevice;
class UsbKvmMcu;

class UsbKvmDevice {
public:
    UsbKvmDevice(const std::string &name);

    MsHal &hal()
    {
        return *m_hal;
    }

    UsbKvmMcu *mcu()
    {
        return m_mcu.get();
    }
    UsbKvmMcu *mcu_boot()
    {
        return m_mcu_boot.get();
    }

    void enter_bootloader();
    void leave_bootloader();
    void delete_mcu();

    Model get_model() const
    {
        if (!m_model)
            return Model::UNKNOWN;
        return m_model.value();
    }
    void set_model(Model model);

    void close_hal();

    ~UsbKvmDevice();

private:
    void set_boot_mcu();
    std::unique_ptr<MsHal> m_hal;
    std::unique_ptr<I2COneDevice> m_i2c1dev_mcu;
    std::unique_ptr<UsbKvmMcu> m_mcu;
    std::unique_ptr<UsbKvmMcu> m_mcu_boot;

    std::optional<Model> m_model;
};

} // namespace usbkvm
