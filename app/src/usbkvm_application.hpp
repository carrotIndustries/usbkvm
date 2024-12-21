#pragma once
#include <gtkmm.h>
#include <gst/gst.h>

namespace usbkvm {

struct DeviceInfo;

class UsbKvmApplication : public Gtk::Application {
protected:
    UsbKvmApplication();

public:
    static Glib::RefPtr<UsbKvmApplication> create();

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

    void on_device_added(const DeviceInfo &devinfo);
    void on_device_removed(const std::string &video_path);

    gboolean monitor_bus_func(GstBus *bus, GstMessage *message);
    static gboolean monitor_bus_func(GstBus *bus, GstMessage *message, gpointer user_data);

    GstDeviceMonitor *m_monitor = nullptr;

    std::map<std::string, DeviceInfo> m_devices;
};

} // namespace usbkvm
