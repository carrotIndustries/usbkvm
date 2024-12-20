#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "usbkvm_device.hpp"
#include "usbkvm_mcu.hpp"


namespace py = pybind11;

struct UsbKvmDevicePy {
    UsbKvmDevicePy(const std::string &name) : m_device(name)
    {
    }

    bool has_mcu()
    {
        return m_device.mcu();
    }

    void close()
    {
        m_device.close_hal();
    }

    void enter_bootloader()
    {
        m_device.enter_bootloader();
    }

    auto get_mcu_info()
    {
        return m_device.mcu()->get_info();
    }

    void set_led(UsbKvmMcu::Led mask, UsbKvmMcu::Led stat)
    {
        m_device.mcu()->set_led(mask, stat);
    }

private:
    UsbKvmDevice m_device;
};

PYBIND11_MODULE(usbkvm_py, m)
{
    py::enum_<Model>(m, "Model")
            .value("UNKNOWN", Model::UNKNOWN)
            .value("USBKVM", Model::USBKVM)
            .value("USBKVM_PRO", Model::USBKVM_PRO);

    py::enum_<UsbKvmMcu::Led>(m, "Led")
            .value("NONE", UsbKvmMcu::Led::NONE)
            .value("USB", UsbKvmMcu::Led::USB)
            .value("HID", UsbKvmMcu::Led::HID)
            .value("HDMI", UsbKvmMcu::Led::HDMI)
            .value("ALL", UsbKvmMcu::Led::ALL);

    py::class_<UsbKvmMcu::Info>(m, "McuInfo")
            .def_readonly("version", &UsbKvmMcu::Info::version)
            .def_readonly("in_bootloader", &UsbKvmMcu::Info::in_bootloader)
            .def_readonly("model", &UsbKvmMcu::Info::model);

    py::class_<UsbKvmMcu>(m, "Mcu")
            .def("get_info", &UsbKvmMcu::get_info)
            .def("enter_bootloader", &UsbKvmMcu::enter_bootloader)
            .def("boot_flash_unlock", &UsbKvmMcu::boot_flash_unlock)
            .def("boot_flash_lock", &UsbKvmMcu::boot_flash_lock)
            .def("boot_flash_erase", &UsbKvmMcu::boot_flash_erase)
            .def("boot_flash_write",
                 [](UsbKvmMcu &mcu, unsigned int offset, const std::vector<uint8_t> data) {
                     if (data.size() != UsbKvmMcu::write_flash_chunk_size)
                         throw std::runtime_error("size mismatch");
                     return mcu.boot_flash_write(offset, std::span<const uint8_t, UsbKvmMcu::write_flash_chunk_size>(
                                                                 data.data(), UsbKvmMcu::write_flash_chunk_size));
                 })
            .def("boot_start_app", &UsbKvmMcu::boot_start_app)
            .def("boot_get_boot_version", &UsbKvmMcu::boot_get_boot_version)
            .def("boot_enter_dfu", &UsbKvmMcu::boot_enter_dfu)
            .def("set_led", &UsbKvmMcu::set_led);
    /*.def("has_mcu", &UsbKvmDevice::has_mcu)
    .def("get_mcu_info", &UsbKvmDevice::get_mcu_info)
    .def("set_led", &UsbKvmDevice::set_led)
    .def("flash_unlock", &UsbKvmDevice::flash_unlock)*/

    py::class_<UsbKvmDevice>(m, "Device")
            .def(py::init<const std::string &>())
            .def("close_hal", &UsbKvmDevice::close_hal)
            .def("mcu", &UsbKvmDevice::mcu, py::return_value_policy::reference)
            /*.def("has_mcu", &UsbKvmDevice::has_mcu)
            .def("get_mcu_info", &UsbKvmDevice::get_mcu_info)
            .def("set_led", &UsbKvmDevice::set_led)
            .def("flash_unlock", &UsbKvmDevice::flash_unlock)*/
            .def("enter_bootloader", &UsbKvmDevice::enter_bootloader);
}
