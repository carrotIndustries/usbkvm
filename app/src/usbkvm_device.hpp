#pragma once
#include <memory>
#include <optional>
#include "model.hpp"

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

    void enter_bootloader();
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
    std::unique_ptr<MsHal> m_hal;
    std::unique_ptr<I2COneDevice> m_i2c1dev_mcu;
    std::unique_ptr<UsbKvmMcu> m_mcu;

    std::optional<Model> m_model;
};
