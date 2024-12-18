#pragma once
#include <string>

class UsbKvmMcu;

class IMcuProvider {
public:
    virtual UsbKvmMcu *get_mcu() = 0;
    virtual void handle_io_error(const std::string &err) = 0;
};
