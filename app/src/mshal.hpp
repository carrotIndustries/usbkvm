#pragma once
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <span>
#include <mutex>
#include "ii2c.hpp"
#include "update_status.hpp"
#include <functional>
#include <chrono>

namespace usbkvm {

struct EEPROMDatecode {
    unsigned int year;
    unsigned int month;
    unsigned int day;

    static const size_t offset;
    static EEPROMDatecode parse(std::span<const uint8_t> data);
    std::string format() const;
    friend auto operator<=>(const EEPROMDatecode &, const EEPROMDatecode &) = default;
};

class MsHal : public II2C {
public:
    MsHal(const std::string &path);
    void i2c_transfer(uint8_t device_addr, std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd) override;
    uint16_t mem_read16be(unsigned int addr);
    uint8_t mem_read8(unsigned int addr);
    void eeprom_read(uint16_t addr, std::span<uint8_t> data);
    void eeprom_write(uint16_t addr, std::span<const uint8_t> data);
    bool update_eeprom(std::function<void(const UpdateProgress &)> progress_cb, std::span<const uint8_t> data);
    template <typename Rep, typename Period> void set_i2c_delay(const std::chrono::duration<Rep, Period> &delay)
    {
        const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(delay);
        set_i2c_delay_ns(ns.count());
    }


    EEPROMDatecode read_eeprom_datecode();

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
    enum class MemoryRegion { RAM = 0, EEPROM = 1 };
    void mem_access(AccessMode mode, MemoryRegion region, unsigned int addr, std::span<uint8_t> data);
    void set_i2c_delay_ns(int64_t delay_ns);
    uintptr_t m_handle;

    std::mutex m_mutex;
};

} // namespace usbkvm
