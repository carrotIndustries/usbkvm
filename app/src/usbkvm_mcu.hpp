#pragma once
#include "bitmask_operators.hpp"
#include "model.hpp"
#include <stdint.h>
#include <array>
#include <mutex>
#include <span>
#include <string>
#include <functional>
#include <stdexcept>

template <auto req_a> struct xfer_t;

namespace usbkvm {

class II2COneDevice;

enum class UsbKvmMcuFirmwareUpdateStatus { BUSY, DONE, ERROR };

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

    bool send_report(const MouseReport &report);

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

    bool send_report(const KeyboardReport &report);

    struct Status {
        bool vga_connected = false;
    };
    Status get_status();

    struct Info {
        unsigned int version;
        bool in_bootloader;
        Model model;
        enum class Valid { INVALID_MAGIC, HEADER_CRC_MISMATCH, APP_CRC_MISMATCH, VALID };
        Valid get_valid() const;
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
    
    std::array<uint32_t, 3> get_unique_id();
    std::string get_serial_number();

    static constexpr size_t write_flash_chunk_size = 256;

    bool boot_flash_unlock();
    bool boot_flash_lock();
    bool boot_flash_erase(unsigned int first_page, unsigned int n_pages);
    bool boot_flash_write(unsigned int offset, std::span<const uint8_t, write_flash_chunk_size> data);
    void boot_start_app();
    uint8_t boot_get_boot_version();
    void boot_enter_dfu();

    using FirmwareUpdateStatus = UsbKvmMcuFirmwareUpdateStatus;
    struct FirmwareUpdateProgress {
        FirmwareUpdateStatus status;
        std::string message;
        float progress = 0;
    };


    bool boot_update_firmware(std::function<void(const FirmwareUpdateProgress &)> progress_cb,
                              std::span<const uint8_t> firmware);

    ~UsbKvmMcu();

private:
    II2COneDevice &m_i2c;
    uint8_t m_seq = 1;
    std::mutex m_mutex;

    template <auto req> xfer_t<req>::Tresp i2c_xfer();
    template <auto req> xfer_t<req>::Tresp i2c_xfer(xfer_t<req>::Treq req_s);
    template <auto req> xfer_t<req>::Tresp i2c_xfer_retry(xfer_t<req>::Treq req_s);

    enum class TransferMode { ONCE, RETRY };
    template <auto req> xfer_t<req>::Tresp i2c_xfer(xfer_t<req>::Treq req_s, TransferMode mode);
};

} // namespace usbkvm

template <> struct enable_bitmask_operators<usbkvm::UsbKvmMcu::MouseReport::Button> {
    static constexpr bool enable = true;
};

template <> struct enable_bitmask_operators<usbkvm::UsbKvmMcu::KeyboardReport::Modifier> {
    static constexpr bool enable = true;
};

template <> struct enable_bitmask_operators<usbkvm::UsbKvmMcu::Led> {
    static constexpr bool enable = true;
};
