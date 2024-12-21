#pragma once
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <span>
#include <mutex>
#include "ii2c.hpp"

namespace usbkvm {

class MsHal : public II2C {
public:
    MsHal(const std::string &name);
    void i2c_transfer(uint8_t device_addr, std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd) override;
    uint16_t mem_read16be(unsigned int addr);
    uint8_t mem_read8(unsigned int addr);

    uint16_t get_input_width();
    uint16_t get_input_height();
    double get_input_fps();
    bool get_has_signal();

    // for python interface
    void close();

    virtual ~MsHal();

    struct IOError : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

private:
    enum class AccessMode : bool { WRITE = true, READ = false };
    void mem_access(AccessMode mode, unsigned int addr, std::span<uint8_t> data);
    uintptr_t m_handle;

    std::mutex m_mutex;
};

} // namespace usbkvm
