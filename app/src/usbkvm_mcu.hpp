#pragma once
#include "bitmask_operators.hpp"
#include "model.hpp"
#include <stdint.h>
#include <array>
#include <mutex>

class II2COneDevice;

class UsbKvmMcu {
public:
    UsbKvmMcu(II2COneDevice &i2c);

    struct MouseReport {
        enum class Button {
            NONE = 0,
            LEFT = (1 << 0),
            RIGHT = (1 << 1),
            MIDDLE = (1 << 2),
            BACKWARD = (1 << 3),
            FORWARD = (1 << 4),
        };
        Button button = Button::NONE;
        double x = 0;
        double y = 0;
        int8_t hscroll = 0;
        int8_t vscroll = 0;
    };

    void send_report(const MouseReport &report);

    struct KeyboardReport {
        enum class Modifier {
            NONE = 0,
            LEFTCTRL = (1 << 0),
            LEFTSHIFT = (1 << 1),
            LEFTALT = (1 << 2),
            LEFTGUI = (1 << 3),
        };
        Modifier mod = Modifier::NONE;
        std::array<uint8_t, 6> keycode = {0};
    };

    void send_report(const KeyboardReport &report);

    struct Status {
        bool vga_connected = false;
    };
    Status get_status();

    struct Info {
        unsigned int version;
        Model model;
    };

    Info get_info();
    static unsigned int get_expected_version();
    void enter_bootloader();

    enum class Led {
        NONE = 0,
        USB = (1 << 0),
        HID = (1 << 1),
        HDMI = (1 << 2),
        ALL = USB | HID | HDMI,
    };
    void set_led(Led mask, Led stat);

    ~UsbKvmMcu();

private:
    II2COneDevice &m_i2c;
    uint8_t m_seq = 0;
    std::mutex m_mutex;
};

template <> struct enable_bitmask_operators<UsbKvmMcu::MouseReport::Button> {
    static constexpr bool enable = true;
};

template <> struct enable_bitmask_operators<UsbKvmMcu::KeyboardReport::Modifier> {
    static constexpr bool enable = true;
};

template <> struct enable_bitmask_operators<UsbKvmMcu::Led> {
    static constexpr bool enable = true;
};
