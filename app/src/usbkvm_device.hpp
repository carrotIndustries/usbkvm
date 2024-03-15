#pragma once
#include <memory>

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

    ~UsbKvmDevice();

private:
    std::unique_ptr<MsHal> m_hal;
    std::unique_ptr<I2COneDevice> m_i2c1dev_mcu;
    std::unique_ptr<UsbKvmMcu> m_mcu;
};
