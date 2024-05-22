#include "mshal.hpp"
#include "mslib.h"
#include <vector>
#include <stdexcept>
#include <string.h>
#include <iostream>
#include <format>

MsHal::MsHal(const std::string &name)
{
    std::vector<char> name_mut(name.begin(), name.end());
    m_handle = MsHalOpen(name_mut.data());
    if (m_handle == 0)
        throw std::runtime_error("MsHalOpen failed");
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
        throw std::runtime_error("MsHalI2CTransfer failed");
}

void MsHal::mem_access(AccessMode mode, unsigned int addr, std::span<uint8_t> data)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    int retval = MsHalMemAccess(m_handle, (int)mode, addr, data.data(), data.size_bytes());
    if (retval)
        throw std::runtime_error("MsHalMemAccess failed");
}

uint16_t MsHal::mem_read16be(unsigned int addr)
{
    std::array<uint8_t, 2> buf;
    mem_access(AccessMode::READ, addr, buf);
    return (std::get<0>(buf) << 8) | (std::get<1>(buf));
}

uint8_t MsHal::mem_read8(unsigned int addr)
{
    uint8_t buf;
    mem_access(AccessMode::READ, addr, {&buf, 1});
    return buf;
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

// doesn't work reliably
bool MsHal::get_has_signal()
{
    std::cout << mem_read8(0xc568) << std::endl;
    return mem_read8(0xc568) & 1;
}

MsHal::~MsHal()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    MsHalClose(m_handle);
}
