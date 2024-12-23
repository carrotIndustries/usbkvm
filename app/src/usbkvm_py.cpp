#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "usbkvm_device.hpp"
#include "usbkvm_mcu.hpp"

namespace py = pybind11;

using namespace usbkvm;

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

    py::class_<UsbKvmDevice>(m, "Device")
            .def(py::init<const std::string &>())
            .def("close_hal", &UsbKvmDevice::close_hal)
            .def("mcu", &UsbKvmDevice::mcu, py::return_value_policy::reference)
            .def("mcu_boot", &UsbKvmDevice::mcu_boot, py::return_value_policy::reference)
            .def("enter_bootloader", &UsbKvmDevice::enter_bootloader);
}
