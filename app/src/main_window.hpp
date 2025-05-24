#pragma once
#include <gtkmm.h>
#include <gst/gst.h>
#include "imcu_provider.hpp"
#include <atomic>
#include <optional>
#include <span>
#include "device_info.hpp"
#include "update_status.hpp"

namespace usbkvm {

class UsbKvmDevice;
class UsbKvmApplication;


class UsbKvmAppWindow : public Gtk::ApplicationWindow, private IMcuProvider {
public:
    UsbKvmAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, UsbKvmApplication &app);
    static UsbKvmAppWindow *create(UsbKvmApplication &app);

    ~UsbKvmAppWindow();

    const std::string &get_last_bus_info() const
    {
        return m_last_bus_info;
    }

    void set_device(const DeviceInfo &devinfo);
    void unset_device();

    const DeviceInfo *get_device_info() const
    {
        if (m_device_info)
            return &m_device_info.value();
        else
            return nullptr;
    }

private:
    UsbKvmApplication &m_app;
    GstElement *m_pipeline = nullptr;
    GstElement *m_videosrc = nullptr;
    GstElement *m_capsfilter = nullptr;
    GstElement *m_jpegdec = nullptr;
    GstElement *m_videocvt = nullptr;
    GstElement *m_videosink = nullptr;
    GstStateChangeReturn ret;
    GstBus *m_bus;
    bool m_has_video = false;

    Gtk::EventBox *m_evbox;
    Glib::RefPtr<Gdk::Cursor> m_blank_cursor;
    Gtk::InfoBar *m_mcu_info_bar;
    Gtk::InfoBar *m_eeprom_info_bar;
    Gtk::Label *m_mcu_info_bar_label;
    Gtk::Button *m_update_firmware_button;
    Gtk::Revealer *m_firmware_update_revealer;
    Gtk::ProgressBar *m_firmware_update_progress_bar;
    Gtk::Label *m_firmware_update_label;
    Gtk::Label *m_eeprom_update_label;
    Gtk::Button *m_update_eeprom_button;
    Gtk::Label *m_update_blurb_label;
    Gtk::Label *m_overlay_label;
    Gtk::HeaderBar *m_headerbar;
    void set_update_buttons_sensitive(bool s);

    void set_overlay_label_text(const std::string &label);

    using FirmwareUpdateStatus = UpdateStatus;
    enum class UpdateType { FIRMWARE, EEPROM };
    UpdateType m_update_type;
    std::atomic<FirmwareUpdateStatus> m_firmware_update_status;
    std::atomic<float> m_firmware_update_progress;
    std::string m_firmware_update_status_string;
    std::mutex m_firmware_update_status_mutex;
    Glib::Dispatcher m_firmware_update_dispatcher;
    void firmware_update_thread();
    void eeprom_update_thread();
    void set_firmware_update_status(FirmwareUpdateStatus status, const std::string &msg);
    void update_firmware_update_status();

    Gtk::Box *m_modifier_box;
    std::map<GdkModifierType, Gtk::ToggleButton *> m_modifier_buttons;
    void update_modifier_buttons();

    Gtk::Label *m_input_status_label;
    enum class UpdateCaptureResolution { NO, YES };
    bool update_input_status(UpdateCaptureResolution update);

    std::unique_ptr<UsbKvmDevice> m_device;
    std::set<guint> m_keys_pressed;
    guint m_modifiers = 0;
    uint32_t m_buttons_pressed = 0;
    double m_cursor_x = 0;
    double m_cursor_y = 0;

    void send_mouse_report(int vscroll_delta = 0);
    void send_keyboard_report();

    void handle_button(GdkEventButton *ev);
    bool handle_key(GdkEventKey *ev);
    void handle_scroll(GdkEventScroll *ev);
    void handle_motion(GdkEventMotion *ev);

    void create_device(const std::string &path);
    void update_firmware();
    void update_eeprom();
    void realize();
    std::span<const uint16_t> m_keycode_map;

    std::pair<int, int> m_capture_resolution = {1280, 720};
    std::set<std::pair<int, int>> m_capture_resolutions;
    bool m_auto_capture_resolution = true;
    void update_auto_capture_resolution(int w, int h);
    void set_capture_resolution(int w, int h);

    Gtk::Button *m_resolution_button = nullptr;
    Gtk::Box *m_capture_resolution_box = nullptr;
    void update_resolution_button();

    void add_capture_resolution(int width, int height);

    class TypeWindow *m_type_window = nullptr;
    UsbKvmMcu *get_mcu() override;

    void handle_io_error(const std::string &err) override;

    std::optional<DeviceInfo> m_device_info;
    std::string m_last_bus_info;
    void mcu_init();
};

} // namespace usbkvm
