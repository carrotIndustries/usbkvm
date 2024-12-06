#include "usbkvm_mcu.hpp"
#include "ii2c.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <thread>
#define I2C_HAVE_BOOTLOADER
#include "../fw/common/Inc/i2c_msg.h"

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

void UsbKvmMcu::send_report(const MouseReport &report)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_mouse_report_t msg = {
            .type = I2C_REQ_MOUSE_REPORT,
            .seq = m_seq++,
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

    i2c_send(m_i2c, msg);
}

void UsbKvmMcu::send_report(const KeyboardReport &report)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_keyboard_report_t msg = {
            .type = I2C_REQ_KEYBOARD_REPORT,
            .seq = m_seq++,
    };

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

    i2c_send(m_i2c, msg);
}

UsbKvmMcu::Info UsbKvmMcu::get_info()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_unknown_t msg = {.type = I2C_REQ_GET_INFO, .seq = m_seq++};
    i2c_resp_info_t resp;
    i2c_send_recv(m_i2c, msg, resp);

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

void UsbKvmMcu::enter_bootloader()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_unknown_t msg = {.type = I2C_REQ_ENTER_BOOTLOADER, .seq = m_seq++};
    i2c_send(m_i2c, msg);
}

UsbKvmMcu::Status UsbKvmMcu::get_status()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_unknown_t msg = {.type = I2C_REQ_GET_STATUS, .seq = m_seq++};
    i2c_resp_status_t resp;
    i2c_send_recv(m_i2c, msg, resp);

    UsbKvmMcu::Status ret;
    ret.vga_connected = resp.status & I2C_STATUS_VGA_CONNECTED;
    return ret;
}

unsigned int UsbKvmMcu::get_expected_version()
{
    return I2C_VERSION;
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

    i2c_req_set_led_t msg = {
            .type = I2C_REQ_SET_LED, .seq = m_seq++, .mask = translate_led(mask), .stat = translate_led(stat)};
    i2c_send(m_i2c, msg);
}

bool UsbKvmMcu::flash_unlock()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_unknown_t msg = {.type = I2C_REQ_FLASH_UNLOCK, .seq = m_seq++};
    i2c_resp_flash_status_t resp;
    i2c_send_recv(m_i2c, msg, resp);

    return resp.success;
}

bool UsbKvmMcu::flash_lock()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_unknown_t msg = {.type = I2C_REQ_FLASH_LOCK, .seq = m_seq++};
    i2c_send(m_i2c, msg);

    return true;
}

bool UsbKvmMcu::flash_erase(unsigned int first_page, unsigned int n_pages)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_flash_erase_t msg = {
            .type = I2C_REQ_FLASH_ERASE,
            .seq = m_seq++,
            .first_page = static_cast<uint8_t>(first_page),
            .n_pages = static_cast<uint8_t>(n_pages),
    };
    i2c_resp_flash_status_t resp;
    i2c_send_recv_retry(m_i2c, msg, resp);

    return resp.success;
}

bool UsbKvmMcu::flash_write(unsigned int offset, std::span<const uint8_t, 256> data)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    i2c_req_flash_write_t msg = {.type = I2C_REQ_FLASH_WRITE, .seq = m_seq++, .offset = static_cast<uint16_t>(offset)};
    static_assert(data.size() == sizeof(msg.data));
    memcpy(msg.data, data.data(), sizeof(msg.data));
    i2c_resp_flash_status_t resp;
    i2c_send_recv_retry(m_i2c, msg, resp);

    return resp.success;
}


UsbKvmMcu::~UsbKvmMcu() = default;
