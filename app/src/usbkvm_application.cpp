#include "usbkvm_application.hpp"
#include "main_window.hpp"
#include "about_dialog.hpp"

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
    // The application has been started, so let's show a window.
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

    signal_shutdown().connect(sigc::mem_fun(*this, &UsbKvmApplication::on_shutdown));
}


void UsbKvmApplication::on_shutdown()
{
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

} // namespace usbkvm
