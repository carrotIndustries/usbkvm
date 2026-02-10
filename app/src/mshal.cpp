#include "mshal.hpp"
#include "mslib.h"
#include <vector>
#include <stdexcept>
#include <string.h>
#include <format>
#include "util.hpp"

namespace usbkvm {

static unsigned int bcd(uint8_t x)
{
    return (x & 0xf) + ((x >> 4) & 0xf) * 10;
}

decltype(EEPROMDatecode::offset) EEPROMDatecode::offset = 12;

EEPROMDatecode EEPROMDatecode::parse(std::span<const uint8_t> data)
{
    if (data.size() < 4)
        throw std::invalid_argument("need at least 4 bytes");
    return {.year = bcd(data[0]) * 100 + bcd(data[1]), .month = bcd(data[2]), .day = bcd(data[3])};
}

std::string EEPROMDatecode::format() const
{
    return std::format("{:04}-{:02}-{:02}", year, month, day);
}

MsHal::MsHal(const std::string &path)
{
    std::vector<char> path_mut(path.begin(), path.end());
    path_mut.push_back(0); // null terminator
    m_handle = MsHalOpen(path_mut.data());
    if (m_handle == 0)
        throw IOError("MsHalOpen failed");
}

void MsHal::i2c_transfer(uint8_t device_addr, std::span<const uint8_t> data_wr, std::span<uint8_t> data_rd)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::vector<uint8_t> data_wr_mut(data_wr.begin(), data_wr.end());
    /*
    std::cout << "i2c xfer ";
    for (auto c : data_wr) {
        std::cout << std::format("{:02x} ", c);
    }
    std::cout << std::endl;
    */
    void *rdbuf = nullptr;
    int retval =
            MsHalI2CTransfer(m_handle, device_addr, data_wr_mut.data(), data_wr_mut.size(), data_rd.size(), &rdbuf);
    if (rdbuf) {
        if (data_rd.size())
            memcpy(data_rd.data(), rdbuf, data_rd.size());
        free(rdbuf);
    }
    if (retval != 0)
        throw IOError("MsHalI2CTransfer failed");
}

void MsHal::mem_access(AccessMode mode, MemoryRegion region, unsigned int addr, std::span<uint8_t> data)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    int retval = MsHalMemAccess(m_handle, (int)mode, (int)region, addr, data.data(), data.size_bytes());
    if (retval)
        throw IOError("MsHalMemAccess failed");
}

uint16_t MsHal::mem_read16be(unsigned int addr)
{
    std::array<uint8_t, 2> buf;
    mem_access(AccessMode::READ, MemoryRegion::RAM, addr, buf);
    return (std::get<0>(buf) << 8) | (std::get<1>(buf));
}

uint8_t MsHal::mem_read8(unsigned int addr)
{
    uint8_t buf;
    mem_access(AccessMode::READ, MemoryRegion::RAM, addr, {&buf, 1});
    return buf;
}

void MsHal::eeprom_read(uint16_t addr, std::span<uint8_t> data)
{
    mem_access(AccessMode::READ, MemoryRegion::EEPROM, addr, data);
}

void MsHal::eeprom_write(uint16_t addr, std::span<const uint8_t> data)
{
    std::vector<decltype(data)::value_type> data_buf;
    data_buf.assign(data.begin(), data.end());
    mem_access(AccessMode::WRITE, MemoryRegion::EEPROM, addr, data_buf);
}

EEPROMDatecode MsHal::read_eeprom_datecode()
{
    std::array<uint8_t, 4> buf;
    eeprom_read(static_cast<uint16_t>(EEPROMDatecode::offset), buf);
    return EEPROMDatecode::parse(buf);
}

bool MsHal::update_eeprom(std::function<void(const UpdateProgress &)> progress_cb, std::span<const uint8_t> data)
{
    using S = UpdateStatus;

    const auto chunk_size = 256;
    if (data.size() % chunk_size) {
        progress_cb({S::ERROR, "data size error"});
        return false;
    }

    const unsigned int n_chunks = data.size() / chunk_size;

    for (unsigned int chunk = 0; chunk < n_chunks; chunk++) {
        const float progress = (chunk + 1) / ((float)n_chunks);
        const unsigned int offset = chunk * chunk_size;
        progress_cb({S::BUSY, "Programming: " + format_m_of_n(offset, data.size()) + " Bytes", progress});

        eeprom_write(offset, data.subspan(offset, chunk_size));
    }

    return true;
}

void MsHal::set_i2c_delay_ns(int64_t delay_ns)
{
    MsHalSetI2CDelay(m_handle, delay_ns);
}

// https://github.com/BertoldVdb/ms-tools/issues/7#issuecomment-1431494947
uint16_t MsHal::get_input_width()
{
    // std::cout << (int)mem_read8(0xc568) << std::endl;
    return mem_read16be(0xc572);
}

uint16_t MsHal::get_input_height()
{
    return mem_read16be(0xc574);
}

double MsHal::get_input_fps()
{
    return mem_read16be(0xc578) / 100.;
}

// https://github.com/BertoldVdb/ms-tools/issues/18#issuecomment-2509151910
bool MsHal::get_has_signal()
{
    return mem_read8(0xc74a) & 1;
}

void MsHal::close()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    MsHalClose(m_handle);
}

MsHal::~MsHal()
{
    close();
}

} // namespace usbkvm
