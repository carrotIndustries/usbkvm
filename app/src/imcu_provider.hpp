#pragma once

class UsbKvmMcu;

class IMcuProvider {
public:
    virtual UsbKvmMcu *get_mcu() = 0;
};
