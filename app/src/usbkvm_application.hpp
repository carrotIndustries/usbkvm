#pragma once
#include <gtkmm.h>
#include <gst/gst.h>
#include "idevices_provider.hpp"
#include "device_info.hpp"

namespace usbkvm {

class DevicesWindow;

class UsbKvmApplication : public Gtk::Application, private IDevicesProvider {
protected:
    UsbKvmApplication();

public:
    static Glib::RefPtr<UsbKvmApplication> create();
    void update_device_info(const DeviceInfo &info);

    std::vector<const DeviceInfo *> get_devices() const override;
    type_signal_devices_changed signal_devices_changed() override
    {
        return m_signal_devices_changed;
    }
    void activate_device(const std::string &video_path) override;

    bool get_force_firmware_update() const
    {
        return m_force_firmware_update;
    }


protected:
    // Override default signal handlers:
    void on_activate() override;
    void on_startup() override;
    void on_shutdown();

private:
    class UsbKvmAppWindow *create_appwindow();
    void on_hide_window(Gtk::Window *window);
    void on_action_quit();
    void on_action_about();
    void on_action_devices();

    void on_device_added(const DeviceInfo &devinfo);
    void on_device_removed(const std::string &video_path);

    gboolean monitor_bus_func(GstBus *bus, GstMessage *message);
    static gboolean monitor_bus_func(GstBus *bus, GstMessage *message, gpointer user_data);

    GstDeviceMonitor *m_monitor = nullptr;

    std::map<std::string, DeviceInfo> m_devices;
    std::map<std::string, unsigned int> m_video_devices;
    bool probe_device(const std::string &video_path, const std::string &video_bus_info,
                      const DeviceInfo::ResolutionList &capture_resolutions);

    DevicesWindow *m_devices_window = nullptr;

    type_signal_devices_changed m_signal_devices_changed;
    bool m_force_firmware_update = false;
};

} // namespace usbkvm
