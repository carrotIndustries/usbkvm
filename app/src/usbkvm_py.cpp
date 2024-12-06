#include <pybind11/pybind11.h>
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
            .def_readonly("model", &UsbKvmMcu::Info::model);

    py::class_<UsbKvmDevicePy>(m, "Device")
            .def(py::init<const std::string &>())
            .def("close", &UsbKvmDevicePy::close)
            .def("has_mcu", &UsbKvmDevicePy::has_mcu)
            .def("get_mcu_info", &UsbKvmDevicePy::get_mcu_info)
            .def("set_led", &UsbKvmDevicePy::set_led)
            .def("enter_bootloader", &UsbKvmDevicePy::enter_bootloader);
}
