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
            .def("flash_unlock", &UsbKvmMcu::flash_unlock)
            .def("flash_lock", &UsbKvmMcu::flash_lock)
            .def("flash_erase", &UsbKvmMcu::flash_erase)
            .def("flash_write",
                 [](UsbKvmMcu &mcu, unsigned int offset, const std::vector<uint8_t> data) {
                     if (data.size() != 256)
                         throw std::runtime_error("size mismatch");
                     return mcu.flash_write(offset, std::span<const uint8_t, 256>(data.data(), 256));
                 })
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
