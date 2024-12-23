#include "devices_window.hpp"
#include "idevices_provider.hpp"
#include "device_info.hpp"

namespace usbkvm {

static void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

class DeviceBox : public Gtk::Box {
public:
    DeviceBox(const DeviceInfo &info, Glib::RefPtr<Gtk::SizeGroup> sg_model)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10), m_info(info)
    {
        property_margin() = 10;
        {
            auto la = Gtk::make_managed<Gtk::Label>(m_info.model);
            la->set_width_chars(strlen("USBKVM Pro"));
            sg_model->add_widget(*la);
            pack_start(*la, false, false, 0);
            la->show();
            la->set_xalign(0);
        }
        {
            auto la = Gtk::make_managed<Gtk::Label>();
            la->set_markup("<tt>" + Glib::Markup::escape_text(m_info.serial) + "</tt>");
            la->set_width_chars(2 * 4 + 1);
            pack_start(*la, false, false, 0);
            la->set_xalign(0);
            la->show();
        }
        {
            auto la = Gtk::make_managed<Gtk::Label>();
            la->get_style_context()->add_class("dim-label");
            la->set_text("Video: " + m_info.video_path + "\nHID: " + m_info.hid_path);
            pack_start(*la, false, false, 0);
            la->set_xalign(0);
            la->show();
        }
    }

    const DeviceInfo m_info;
};

DevicesWindow::DevicesWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                             IDevicesProvider &devices_provider)
    : Gtk::Window(cobject), m_devices_provider(devices_provider)
{
    x->get_widget("listbox", m_listbox);
    m_sg_model = Gtk::SizeGroup::create(Gtk::SIZE_GROUP_HORIZONTAL);
    m_listbox->set_header_func(sigc::ptr_fun(&header_func_separator));
    m_listbox->signal_row_activated().connect([this](Gtk::ListBoxRow *row) {
        auto &child = dynamic_cast<DeviceBox &>(*row->get_child());
        m_devices_provider.activate_device(child.m_info.video_path);
    });
    m_devices_provider.signal_devices_changed().connect(sigc::mem_fun(*this, &DevicesWindow::update_devices));
}


void DevicesWindow::update_devices()
{
    for (auto child : m_listbox->get_children()) {
        delete child;
    }
    for (auto dev : m_devices_provider.get_devices()) {
        auto box = Gtk::make_managed<DeviceBox>(*dev, m_sg_model);
        box->show();
        m_listbox->append(*box);
    }
}

DevicesWindow *DevicesWindow::create(IDevicesProvider &devices_provider)
{
    DevicesWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/usbkvm/devices_window.ui");
    x->get_widget_derived("window", w, devices_provider);
    return w;
}

} // namespace usbkvm
