#include "usbkvm_application.hpp"
#include "main_window.hpp"
#include "about_dialog.hpp"
#include <codecvt>
#include <locale>
#include <iostream>
#include <hidapi.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

namespace usbkvm {

UsbKvmApplication::UsbKvmApplication() : Gtk::Application("net.carrotindustries.usbkvm", Gio::APPLICATION_FLAGS_NONE)
{
}

Glib::RefPtr<UsbKvmApplication> UsbKvmApplication::create()
{
    return Glib::RefPtr<UsbKvmApplication>(new UsbKvmApplication());
}

UsbKvmAppWindow *UsbKvmApplication::create_appwindow()
{
    auto appwindow = UsbKvmAppWindow::create(*this);

    // Make sure that the application runs for as long this window is still
    // open.
    add_window(*appwindow);

    // Gtk::Application::add_window() connects a signal handler to the window's
    // signal_hide(). That handler removes the window from the application.
    // If it's the last window to be removed, the application stops running.
    // Gtk::Window::set_application() does not connect a signal handler, but is
    // otherwise equivalent to Gtk::Application::add_window().

    // Delete the window when it is hidden.
    appwindow->signal_hide().connect(sigc::bind(sigc::mem_fun(*this, &UsbKvmApplication::on_hide_window), appwindow));

    return appwindow;
}

void UsbKvmApplication::on_activate()
{
    if (auto win = get_active_window()) {
        win->present();
        return;
    }
    auto appwindow = create_appwindow();
    appwindow->present();
}


void UsbKvmApplication::on_startup()
{
    // Call the base class's implementation.
    Gtk::Application::on_startup();

    add_action("quit", sigc::mem_fun(*this, &UsbKvmApplication::on_action_quit));
    add_action("about", sigc::mem_fun(*this, &UsbKvmApplication::on_action_about));

    auto cssp = Gtk::CssProvider::create();
    cssp->load_from_resource("/net/carrotIndustries/usbkvm/usbkvm.css");
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    Gtk::IconTheme::get_default()->add_resource_path("/net/carrotIndustries/usbkvm/icons");

    {
        m_monitor = gst_device_monitor_new();

        auto bus = gst_device_monitor_get_bus(m_monitor);
        gst_device_monitor_add_filter(m_monitor, "Video/Source", NULL);
        gst_bus_add_watch(bus, &UsbKvmApplication::monitor_bus_func, this);
        gst_object_unref(bus);
        gst_device_monitor_start(m_monitor);
    }


    signal_shutdown().connect(sigc::mem_fun(*this, &UsbKvmApplication::on_shutdown));
}


void UsbKvmApplication::on_shutdown()
{
    gst_device_monitor_stop(m_monitor);
    gst_object_unref(m_monitor);
    gst_deinit();
}

void UsbKvmApplication::on_hide_window(Gtk::Window *window)
{
    delete window;
}


void UsbKvmApplication::on_action_quit()
{
    // Gio::Application::quit() will make Gio::Application::run() return,
    // but it's a crude way of ending the program. The window is not removed
    // from the application. Neither the window's nor the application's
    // destructors will be called, because there will be remaining reference
    // counts in both of them. If we want the destructors to be called, we
    // must remove the window from the application. One way of doing this
    // is to hide the window. See comment in create_appwindow().
    auto windows = get_windows();
    for (auto win : windows)
        win->close();
}

void UsbKvmApplication::on_action_about()
{
    AboutDialog dia;
    auto win = get_active_window();
    if (win)
        dia.set_transient_for(*win);
    dia.run();
}


gboolean UsbKvmApplication::monitor_bus_func(GstBus *bus, GstMessage *message, gpointer pself)
{
    auto self = reinterpret_cast<UsbKvmApplication *>(pself);
    return self->monitor_bus_func(bus, message);
}

static const std::string s_device_name = "USBKVM";


static std::string wide_string_to_string(const std::wstring &wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

static std::string struct_get_string(const GstStructure *props, const char *name)
{
    if (auto r = gst_structure_get_string(props, name))
        return r;
    else
        return "";
}

static std::string get_path(const GstStructure *props)
{
    std::string path = struct_get_string(props, "api.v4l2.path");
    if (path.empty())
        path = struct_get_string(props, "device.path");
    return path;
}

static std::string get_bus_info(const std::string &path)
{
    auto fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd == -1)
        return "";

    char buf[256] = {0};
    auto res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
    if (res < 0) {
        close(fd);
        return "";
    }
    else {
        close(fd);
        std::string buf_s = buf;
        auto slash_pos = buf_s.find('/');
        if (slash_pos == std::string::npos)
            return "";
        return buf_s.substr(0, slash_pos);
    }
}

static gboolean handle_cap(GstCapsFeatures *features, GstStructure *structure, gpointer user_data)
{
    std::string name = gst_structure_get_name(structure);
    auto &ress = *(reinterpret_cast<DeviceInfo::ResolutionList *>(user_data));
    if (name == "image/jpeg") {
        gint width, height;
        if (gst_structure_get_int(structure, "width", &width) && gst_structure_get_int(structure, "height", &height)) {
            ress.emplace_back(width, height);
        }
    }
    return true;
}

gboolean UsbKvmApplication::monitor_bus_func(GstBus *bus, GstMessage *message)
{
    GstDevice *device;
    gchar *name;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_DEVICE_ADDED: {
        gst_message_parse_device_added(message, &device);
        name = gst_device_get_display_name(device);
        g_print("Device added: %s\n", name);
        std::string name_str = name;
        if (name_str.starts_with(s_device_name)) {
            {
                DeviceInfo::ResolutionList capture_resolutions;
                {
                    auto caps = gst_device_get_caps(device);
                    gst_caps_foreach(caps, &handle_cap, &capture_resolutions);
                    gst_caps_unref(caps);
                }

                std::string video_path;
                std::string video_bus_info;

                auto props = gst_device_get_properties(device);

                video_path = get_path(props);

                video_bus_info = struct_get_string(props, "api.v4l2.cap.bus_info");
                if (video_bus_info.empty())
                    video_bus_info = struct_get_string(props, "v4l2.device.bus_info");


                if (video_path.size() && video_bus_info.size()) {
                    std::cout << "found video " << video_path << " at " << video_bus_info << std::endl;
                    auto devinfos = hid_enumerate(0x534d, 0x2109);
                    for (const hid_device_info *dev = devinfos; dev != nullptr; dev = dev->next) {
                        if (wide_string_to_string(dev->product_string) == s_device_name) {
                            auto hid_bus_info = get_bus_info(dev->path);
                            if (hid_bus_info == video_bus_info) {
                                std::cout << video_path << " " << dev->path << " " << get_bus_info(dev->path)
                                          << std::endl;
                                on_device_added({.video_path = video_path,
                                                 .hid_path = dev->path,
                                                 .bus_info = get_bus_info(dev->path),
                                                 .capture_resolutions = capture_resolutions});
                            }
                        }
                    }
                    hid_free_enumeration(devinfos);
                }


                gst_structure_free(props);
            }
        }
        g_free(name);
        gst_object_unref(device);
    } break;
    case GST_MESSAGE_DEVICE_REMOVED: {
        gst_message_parse_device_removed(message, &device);
        name = gst_device_get_display_name(device);
        auto props = gst_device_get_properties(device);
        auto path = get_path(props);
        std::string name_s = name;
        std::cout << "Device removed: " << name_s << " " << path << std::endl;
        on_device_removed(path);
        g_free(name);
        gst_object_unref(device);
        gst_structure_free(props);

    } break;
    default:
        break;
    }

    return G_SOURCE_CONTINUE;
}

void UsbKvmApplication::on_device_added(const DeviceInfo &devinfo)
{
    m_devices.emplace(devinfo.video_path, devinfo);
    // try to use last window
    for (auto win2 : get_windows()) {
        auto &win = dynamic_cast<UsbKvmAppWindow &>(*win2);
        if (win.get_device_info() == nullptr
            && (win.get_last_bus_info() == devinfo.bus_info || win.get_last_bus_info().empty())) {
            win.set_device(devinfo);
            return;
        }
    }
    // try any free window
    for (auto win2 : get_windows()) {
        auto &win = dynamic_cast<UsbKvmAppWindow &>(*win2);
        if (win.get_device_info() == nullptr) {
            win.set_device(devinfo);
            return;
        }
    }
    // no window, create new
    auto appwindow = create_appwindow();
    appwindow->set_device(devinfo);
    appwindow->present();
}

void UsbKvmApplication::on_device_removed(const std::string &video_path)
{
    for (auto win2 : get_windows()) {
        auto &win = dynamic_cast<UsbKvmAppWindow &>(*win2);
        if (auto info = win.get_device_info()) {
            if (info->video_path == video_path) {
                win.unset_device();
                break;
            }
        }
    }
    m_devices.erase(video_path);
}


} // namespace usbkvm