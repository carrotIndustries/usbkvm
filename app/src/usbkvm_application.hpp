#pragma once
#include <gtkmm.h>
#include <gst/gst.h>
#include "idevices_provider.hpp"

namespace usbkvm {

struct DeviceInfo;
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
    std::set<std::string> m_video_devices;

    DevicesWindow *m_devices_window = nullptr;

    type_signal_devices_changed m_signal_devices_changed;
};

} // namespace usbkvm
