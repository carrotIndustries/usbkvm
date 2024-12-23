#pragma once
#include <gtkmm.h>
#include <atomic>

namespace usbkvm {

class IDevicesProvider;

class DevicesWindow : public Gtk::Window {
public:
    DevicesWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IDevicesProvider &devices_provider);
    static DevicesWindow *create(IDevicesProvider &devices_provider);

private:
    IDevicesProvider &m_devices_provider;
    Gtk::ListBox *m_listbox = nullptr;
    Glib::RefPtr<Gtk::SizeGroup> m_sg_model;


    void update_devices();
};

} // namespace usbkvm
