#include "usbkvm_mcu.hpp"
#include "ii2c.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <thread>
#include "i2c_msg_boot.h"
#include "i2c_msg_app.h"

#define TU_BIT(n) (1UL << (n))

typedef enum {
    MOUSE_BUTTON_LEFT = TU_BIT(0),     ///< Left button
    MOUSE_BUTTON_RIGHT = TU_BIT(1),    ///< Right button
    MOUSE_BUTTON_MIDDLE = TU_BIT(2),   ///< Middle button
    MOUSE_BUTTON_BACKWARD = TU_BIT(3), ///< Backward button,
    MOUSE_BUTTON_FORWARD = TU_BIT(4),  ///< Forward button,
} hid_mouse_button_bm_t;

/// Keyboard modifier codes bitmap
typedef enum {
    KEYBOARD_MODIFIER_LEFTCTRL = TU_BIT(0),   ///< Left Control
    KEYBOARD_MODIFIER_LEFTSHIFT = TU_BIT(1),  ///< Left Shift
    KEYBOARD_MODIFIER_LEFTALT = TU_BIT(2),    ///< Left Alt
    KEYBOARD_MODIFIER_LEFTGUI = TU_BIT(3),    ///< Left Window
    KEYBOARD_MODIFIER_RIGHTCTRL = TU_BIT(4),  ///< Right Control
    KEYBOARD_MODIFIER_RIGHTSHIFT = TU_BIT(5), ///< Right Shift
    KEYBOARD_MODIFIER_RIGHTALT = TU_BIT(6),   ///< Right Alt
    KEYBOARD_MODIFIER_RIGHTGUI = TU_BIT(7)    ///< Right Window
} hid_keyboard_modifier_bm_t;

UsbKvmMcu::UsbKvmMcu(II2COneDevice &i2c) : m_i2c(i2c)
{
}

static int16_t scale_pos(double d)
{
    return std::clamp((int)(d * 32767), 0, 32767);
}

template <typename T> void i2c_send(II2COneDevice &i2c, const T &msg)
{
    auto pmsg = reinterpret_cast<const uint8_t *>(&msg);
    i2c.i2c_transfer({pmsg, sizeof(T)}, {});
}

template <typename T> void i2c_recv(II2COneDevice &i2c, uint8_t seq, T &resp)
{
    static uint8_t nop = 0;
    while (true) {
        i2c.i2c_transfer({&nop, 1}, {reinterpret_cast<uint8_t *>(&resp), sizeof(T)});
        if (resp.seq == seq) {
            return;
        }
    }
}

template <typename Ts, typename Tr> void i2c_send_recv(II2COneDevice &i2c, const Ts &req, Tr &resp)
{
    i2c_send(i2c, req);
    i2c_recv(i2c, req.seq, resp);
}

template <typename Ts, typename Tr> void i2c_send_recv_retry(II2COneDevice &i2c, const Ts &req, Tr &resp)
{
    i2c_send(i2c, req);
    for (size_t i = 0; i < 100; i++) {
        try {
            i2c_recv(i2c, req.seq, resp);
            return;
        }
        catch (std::runtime_error &err) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
    }
    throw std::runtime_error("I2C receive timeout");
}

bool UsbKvmMcu::send_report(const MouseReport &report)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_mouse_report_t msg = {
            .button = 0,
            .x = scale_pos(report.x),
            .y = scale_pos(report.y),
            .h = report.hscroll,
            .v = report.vscroll,
    };

#define TRANSLATE_BUTTON(b)                                                                                            \
    if ((report.button & MouseReport::Button::b) != MouseReport::Button::NONE) {                                       \
        msg.button |= MOUSE_BUTTON_##b;                                                                                \
    }

    TRANSLATE_BUTTON(LEFT)
    TRANSLATE_BUTTON(RIGHT)
    TRANSLATE_BUTTON(MIDDLE)
    TRANSLATE_BUTTON(BACKWARD)
    TRANSLATE_BUTTON(FORWARD)
#undef TRANSLATE_BUTTON

    return i2c_xfer<i2c_req_t::MOUSE_REPORT>(msg).success;
}

bool UsbKvmMcu::send_report(const KeyboardReport &report)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_keyboard_report_t msg = {0};

#define TRANSLATE_MOD(b)                                                                                               \
    if ((report.mod & KeyboardReport::Modifier::b) != KeyboardReport::Modifier::NONE) {                                \
        msg.mod |= KEYBOARD_MODIFIER_##b;                                                                              \
    }

    TRANSLATE_MOD(LEFTCTRL)
    TRANSLATE_MOD(LEFTSHIFT)
    TRANSLATE_MOD(LEFTALT)
    TRANSLATE_MOD(LEFTGUI)
#undef TRANSLATE_MOD

    for (size_t i = 0; i < report.keycode.size(); i++) {
        msg.keycode[i] = report.keycode.at(i);
    }

    return i2c_xfer<i2c_req_t::KEYBOARD_REPORT>(msg).success;
}

template <auto req> xfer_t<req>::Tresp UsbKvmMcu::i2c_xfer(xfer_t<req>::Treq req_s, TransferMode mode)
{
    req_s.seq = m_seq++;
    req_s.type = static_cast<uint8_t>(req);
    if constexpr (std::is_void_v<typename xfer_t<req>::Tresp>) {
        i2c_send(m_i2c, req_s);
    }
    else {
        typename xfer_t<req>::Tresp resp;
        if (mode == TransferMode::ONCE)
            i2c_send_recv(m_i2c, req_s, resp);
        else
            i2c_send_recv_retry(m_i2c, req_s, resp);
        return resp;
    }
}

template <auto req> xfer_t<req>::Tresp UsbKvmMcu::i2c_xfer()
{
    return i2c_xfer<req>({});
}

template <auto req> xfer_t<req>::Tresp UsbKvmMcu::i2c_xfer(xfer_t<req>::Treq req_s)
{
    return i2c_xfer<req>(req_s, TransferMode::ONCE);
}

template <auto req> xfer_t<req>::Tresp UsbKvmMcu::i2c_xfer_retry(xfer_t<req>::Treq req_s)
{
    return i2c_xfer<req>(req_s, TransferMode::RETRY);
}

UsbKvmMcu::Info UsbKvmMcu::get_info()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    const auto resp = i2c_xfer<i2c_req_common_t::GET_INFO>({});

    Model model = Model::UNKNOWN;
    switch (resp.model) {
    case I2C_MODEL_USBKVM:
        model = Model::USBKVM;
        break;
    case I2C_MODEL_USBKVM_PRO:
        model = Model::USBKVM_PRO;
        break;
    }

    return {
            .version = static_cast<unsigned int>(resp.version & I2C_VERSION_MASK),
            .in_bootloader = static_cast<bool>(resp.version & I2C_VERSION_BOOT),
            .model = model,
    };
}

UsbKvmMcu::Info::Valid UsbKvmMcu::Info::get_valid() const
{
    if (version == I2C_VERSION_BOOT_APP_INVALID_MAGIC)
        return Valid::INVALID_MAGIC;
    else if (version == I2C_VERSION_BOOT_APP_HEADER_CRC_MISMATCH)
        return Valid::HEADER_CRC_MISMATCH;
    else if (version == I2C_VERSION_BOOT_APP_CRC_MISMATCH)
        return Valid::APP_CRC_MISMATCH;
    return Valid::VALID;
}

void UsbKvmMcu::enter_bootloader()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_xfer<i2c_req_t::ENTER_BOOTLOADER>();
}

UsbKvmMcu::Status UsbKvmMcu::get_status()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto resp = i2c_xfer<i2c_req_t::GET_STATUS>();

    UsbKvmMcu::Status ret;
    ret.vga_connected = resp.status & I2C_STATUS_VGA_CONNECTED;
    return ret;
}

unsigned int UsbKvmMcu::get_expected_version()
{
    return I2C_APP_VERSION;
}

static uint8_t translate_led(UsbKvmMcu::Led led)
{
    uint8_t r = 0;
#define X(l)                                                                                                           \
    if ((led & UsbKvmMcu::Led::l) != UsbKvmMcu::Led::NONE)                                                             \
        r |= I2C_LED_##l;
    X(HID)
    X(HDMI)
    X(USB)
#undef X

    return r;
}

void UsbKvmMcu::set_led(Led mask, Led stat)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_xfer<i2c_req_t::SET_LED>({.mask = translate_led(mask), .stat = translate_led(stat)});
    ;
}

bool UsbKvmMcu::boot_flash_unlock()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto resp = i2c_xfer<i2c_req_boot_t::FLASH_UNLOCK>();

    return resp.success;
}

bool UsbKvmMcu::boot_flash_lock()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_xfer<i2c_req_boot_t::FLASH_LOCK>();

    return true;
}

bool UsbKvmMcu::boot_flash_erase(unsigned int first_page, unsigned int n_pages)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto resp = i2c_xfer_retry<i2c_req_boot_t::FLASH_ERASE>(
            {.first_page = static_cast<uint8_t>(first_page), .n_pages = static_cast<uint8_t>(n_pages)});

    return resp.success;
}

bool UsbKvmMcu::boot_flash_write(unsigned int offset, std::span<const uint8_t, write_flash_chunk_size> data)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    static constexpr auto req_type = i2c_req_boot_t::FLASH_WRITE;
    xfer_t<req_type>::Treq msg = {.offset = static_cast<uint16_t>(offset)};
    static_assert(data.size() == sizeof(msg.data));
    memcpy(msg.data, data.data(), sizeof(msg.data));

    auto resp = i2c_xfer_retry<req_type>(msg);

    return resp.success;
}

void UsbKvmMcu::boot_start_app()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_xfer<i2c_req_boot_t::START_APP>();
}

uint8_t UsbKvmMcu::boot_get_boot_version()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto resp = i2c_xfer<i2c_req_boot_t::GET_BOOT_VERSION>();

    return resp.version;
}

void UsbKvmMcu::boot_enter_dfu()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_xfer<i2c_req_boot_t::ENTER_DFU>();
}

static std::string format_m_of_n(unsigned int m, unsigned int n)
{
    auto n_str = std::to_string(n);
    auto digits_max = n_str.size();
    auto m_str = std::to_string(m);
    std::string prefix;
    for (size_t i = 0; i < (digits_max - (int)m_str.size()); i++) {
        prefix += " ";
    }
    return prefix + m_str + "/" + n_str;
}

bool UsbKvmMcu::boot_update_firmware(std::function<void(const FirmwareUpdateProgress &)> progress_cb,
                                     std::span<const uint8_t> firmware)
{
    using S = FirmwareUpdateStatus;
    if (!get_info().in_bootloader) {
        progress_cb({S::ERROR, "not in bootloader"});
        return false;
    }


    if (firmware.size() % UsbKvmMcu::write_flash_chunk_size) {
        progress_cb({S::ERROR, "firmware size error"});
        return false;
    }

    const unsigned int page_size = 1024;
    const unsigned int n_pages = (firmware.size() + page_size - 1) / page_size;
    const unsigned int n_chunks = firmware.size() / UsbKvmMcu::write_flash_chunk_size;
    progress_cb({S::BUSY, "Erasing…"});

    if (!boot_flash_unlock()) {
        progress_cb({S::ERROR, "flash unlock"});
        return false;
    }
    if (!boot_flash_erase(8, n_pages)) {
        progress_cb({S::ERROR, "flash erase"});
        return false;
    }


    for (unsigned int chunk = 0; chunk < n_chunks; chunk++) {
        const float progress = chunk / ((float)n_chunks);
        static const unsigned int flash_base = 0x8002000;
        const unsigned int offset = chunk * UsbKvmMcu::write_flash_chunk_size;
        progress_cb({S::BUSY, "Programming: " + format_m_of_n(offset, firmware.size()) + " Bytes", progress});

        const unsigned int flash_offset = flash_base + offset;
        if (!boot_flash_write(flash_offset, std::span<const uint8_t, 256>(firmware.data() + offset, 256))) {
            progress_cb({S::ERROR, "flash program"});
            return false;
        }
    }

    progress_cb({S::BUSY, "Finishing…", 1.});

    if (!boot_flash_lock()) {
        progress_cb({S::ERROR, "lock"});
        return false;
    }

    using namespace std::chrono_literals;

    {
        const auto info = get_info();
        if (info.version != get_expected_version()) {
            progress_cb({S::ERROR, "unexpected version after update"});
            return false;
        }
    }
    boot_start_app();
    std::this_thread::sleep_for(200ms);

    {
        const auto info = get_info();
        if (info.in_bootloader) {
            progress_cb({S::ERROR, "stuck in bootloader"});
            return false;
        }
        if (info.version != UsbKvmMcu::get_expected_version()) {
            progress_cb({S::ERROR, "unexpected version after starting app"});
            return false;
        }
    }

    progress_cb({S::BUSY, "Done", 1.});


    return true;
}

UsbKvmMcu::~UsbKvmMcu() = default;
