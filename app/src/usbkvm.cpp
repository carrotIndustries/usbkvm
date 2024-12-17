#include "main_window.hpp"
#include <gst/gst.h>


int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);
    auto app = Gtk::Application::create(argc, argv, "net.carrotindustries.usbkvm");
    auto cssp = Gtk::CssProvider::create();
    cssp->load_from_resource("/net/carrotIndustries/usbkvm/usbkvm.css");
    Gtk::StyleContext::add_provider_for_screen(Gdk::Screen::get_default(), cssp,
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    auto win = MainWindow::create();
    return app->run(*win);
    gst_deinit();
}
