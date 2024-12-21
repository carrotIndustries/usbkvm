#pragma once
#include <gtkmm.h>

namespace usbkvm {

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
};

} // namespace usbkvm
