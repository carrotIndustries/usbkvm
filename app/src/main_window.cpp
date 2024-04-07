#include "main_window.hpp"
#include <cassert>
#include <iostream>
#include "keymap.h"
#include <format>
#include "usbkvm_mcu.hpp"
#include "usbkvm_device.hpp"
#include "mshal.hpp"
#include "type_window.hpp"

static void end_stream_cb(GstBus *bus, GstMessage *message, GstElement *pipeline)
{
    g_print("End of stream\n");
}

static void state_cb(GstBus *bus, GstMessage *message, GstElement *pipeline)
{
    const GstStructure *s;

    s = gst_message_get_structure(message);

    /* We only care about state changed on the pipeline */
    if (s && GST_MESSAGE_SRC(message) == GST_OBJECT_CAST(pipeline)) {
        GstState old, newstate, pending;

        gst_message_parse_state_changed(message, &old, &newstate, &pending);

        std::cout << "state ch " << gst_element_state_get_name(old) << " -> " << gst_element_state_get_name(newstate)
                  << std::endl;
    }
}


void MainWindow::handle_button(GdkEventButton *ev)
{
    const auto mask = 1 << ev->button;
    bool handled = false;

    if (ev->type == GDK_BUTTON_PRESS) {
        m_buttons_pressed |= mask;
        handled = true;
    }
    else if (ev->type == GDK_BUTTON_RELEASE) {
        m_buttons_pressed &= ~mask;
        handled = true;
    }

    if (handled)
        send_mouse_report();
}

void MainWindow::send_mouse_report(int scroll_delta)
{
    if (!(m_device && m_device->mcu()))
        return;
    auto &mcu = *m_device->mcu();

    UsbKvmMcu::MouseReport report;
    using B = UsbKvmMcu::MouseReport::Button;

    if (m_buttons_pressed & (1 << 1))
        report.button |= B::LEFT;
    if (m_buttons_pressed & (1 << 2))
        report.button |= B::MIDDLE;
    if (m_buttons_pressed & (1 << 3))
        report.button |= B::RIGHT;
    if (m_buttons_pressed & (1 << 8))
        report.button |= B::BACKWARD;
    if (m_buttons_pressed & (1 << 9))
        report.button |= B::FORWARD;

    report.x = m_cursor_x;
    report.y = m_cursor_y;
    report.vscroll = scroll_delta;

    mcu.send_report(report);
}


static int gdk_key_to_hid(int keyval)
{
    if (keyval < code_map_x11_to_usb_len) {
        auto h = code_map_x11_to_usb[keyval];
        if (h == 135)  // the wrong underscore
            return 45; // the right underscore
        return h;
    }
    return 0;
}

void MainWindow::send_keyboard_report()
{
    if (!(m_device && m_device->mcu()))
        return;
    auto &mcu = *m_device->mcu();

    UsbKvmMcu::KeyboardReport report;
    using M = UsbKvmMcu::KeyboardReport::Modifier;
    if (m_modifiers & GDK_SHIFT_MASK)
        report.mod |= M::LEFTSHIFT;
    if (m_modifiers & GDK_CONTROL_MASK)
        report.mod |= M::LEFTCTRL;
    if (m_modifiers & GDK_MOD1_MASK)
        report.mod |= M::LEFTALT;
    if (m_modifiers & GDK_MOD4_MASK)
        report.mod |= M::LEFTGUI;

    size_t i = 0;

    for (auto k : m_keys_pressed) {
        if (i > report.keycode.size()) {
            break;
        }
        report.keycode.at(i) = gdk_key_to_hid(k);
        i++;
    }

    mcu.send_report(report);
}


void MainWindow::handle_scroll(GdkEventScroll *ev)
{
    switch (ev->direction) {
    case GDK_SCROLL_DOWN:
        send_mouse_report(-1);
        break;

    case GDK_SCROLL_UP:
        send_mouse_report(1);
        break;

    default:;
    }
}


bool MainWindow::handle_key(GdkEventKey *ev)
{
    auto last_keys = m_keys_pressed;
    auto last_mod = m_modifiers;

    static const std::map<int, GdkModifierType> mod_map{
            {GDK_KEY_Shift_L, GDK_SHIFT_MASK},
            {GDK_KEY_Control_L, GDK_CONTROL_MASK},
            {GDK_KEY_Alt_L, GDK_MOD1_MASK},
            {GDK_KEY_Super_L, GDK_MOD4_MASK},
    };

    if (mod_map.contains(ev->keyval)) {
        auto mod = mod_map.at(ev->keyval);
        if (ev->type == GDK_KEY_PRESS)
            m_modifiers |= mod;
        else if (ev->type == GDK_KEY_RELEASE)
            m_modifiers &= ~mod;
    }
    else {
        if (ev->type == GDK_KEY_PRESS)
            m_keys_pressed.insert(ev->keyval);
        else if (ev->type == GDK_KEY_RELEASE)
            m_keys_pressed.erase(ev->keyval);
    }
    update_modifier_buttons();

    if (m_keys_pressed == last_keys && last_mod == m_modifiers)
        return true;

    send_keyboard_report();
    return true;
}

static bool in_area(double x)
{
    return x >= 0 && x <= 1;
}

void MainWindow::handle_motion(GdkEventMotion *ev)
{

    auto alloc = m_evbox->get_allocation();
    const double alloc_aspect = ((double)alloc.get_width()) / alloc.get_height();
    const double capture_aspect = ((double)m_capture_resolution.first) / m_capture_resolution.second;


    double xd = ((double)ev->x / alloc.get_width());
    double yd = ((double)ev->y / alloc.get_height());
    if (alloc_aspect > capture_aspect) {
        auto f = (alloc_aspect / capture_aspect);
        xd = (xd - .5) * f + .5;
    }
    else if (alloc_aspect < capture_aspect) {
        auto f = (capture_aspect / alloc_aspect);
        yd = (yd - .5) * f + .5;
    }


    if ((in_area(xd) != in_area(m_cursor_x)) || (in_area(yd) != in_area(m_cursor_y))) {
        if (in_area(xd) && in_area(yd)) {
            m_evbox->get_window()->set_cursor(m_blank_cursor);
        }
        else {
            m_evbox->get_window()->set_cursor();
        }
    }
    m_cursor_x = xd;
    m_cursor_y = yd;

    send_mouse_report();

    m_evbox->grab_focus();
}

void MainWindow::update_modifier_buttons()
{
    for (auto &[mod, bu] : m_modifier_buttons) {
        bu->set_active(m_modifiers & mod);
    }
}

gboolean MainWindow::monitor_bus_func(GstBus *bus, GstMessage *message, gpointer pself)
{
    auto self = reinterpret_cast<MainWindow *>(pself);
    return self->monitor_bus_func(bus, message);
}

gboolean MainWindow::handle_cap(GstCapsFeatures *features, GstStructure *structure, gpointer user_data)
{
    auto self = reinterpret_cast<MainWindow *>(user_data);
    return self->handle_cap(features, structure);
}

static std::string format_resolution(int width, int height)
{
    std::string width_str = std::to_string(width);
    std::string prefix;
    int npad = 4 - width_str.size();
    for (int i = 0; i < npad; i++) {
        prefix += " ";
    }
    return std::format("{}{}×{}", prefix, width_str, height);
}

gboolean MainWindow::handle_cap(GstCapsFeatures *features, GstStructure *structure)
{
    std::string name = gst_structure_get_name(structure);
    if (name == "image/jpeg") {
        gint width, height;
        if (gst_structure_get_int(structure, "width", &width) && gst_structure_get_int(structure, "height", &height)) {
            m_capture_resolutions.emplace(width, height);
            auto rb = Gtk::make_managed<Gtk::RadioButton>(format_resolution(width, height));
            {
                auto children = m_capture_resolution_box->get_children();
                if (auto first = dynamic_cast<Gtk::RadioButton *>(children.front())) {
                    rb->join_group(*first);
                }
            }
            rb->show();
            rb->signal_toggled().connect([this, rb, width, height] {
                if (!rb->get_active())
                    return;
                m_auto_capture_resolution = false;
                set_capture_resolution(width, height);
            });
            m_capture_resolution_box->pack_start(*rb, false, false, 0);
        }
    }
    return true;
}

void MainWindow::set_capture_resolution(int w, int h)
{
    if (m_capture_resolution == std::make_pair(w, h))
        return;
    m_capture_resolution = {w, h};
    auto caps = gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, w, "height", G_TYPE_INT, h, NULL);
    g_object_set(m_capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);
    update_resolution_button();
}

#ifdef G_OS_WIN32
static const std::string s_device_name = "USB Video";
#else
static const std::string s_device_name = "USBKVM";
#endif

gboolean MainWindow::monitor_bus_func(GstBus *bus, GstMessage *message)
{
    GstDevice *device;
    gchar *name;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_DEVICE_ADDED: {
        gst_message_parse_device_added(message, &device);
        name = gst_device_get_display_name(device);
        g_print("Device added: %s\n", name);
        std::string name_str = name;
        g_free(name);
        if (name_str == s_device_name) {
            std::cout << "found usbkvm" << std::endl;
            {
                auto caps = gst_device_get_caps(device);
                gst_caps_foreach(caps, &MainWindow::handle_cap, this);
                gst_caps_unref(caps);
            }
#ifdef G_OS_WIN32
            g_object_set(m_videosrc, "device-name", s_device_name.c_str(), NULL);
#else
            {
                auto props = gst_device_get_properties(device);
                auto path = gst_structure_get_string(props, "api.v4l2.path");
                if (path) {
                    g_object_set(m_videosrc, "device", path, NULL);
                }

                gst_structure_free(props);
            }
#endif
            create_device("USBKVM");
            auto ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE) {
                g_print("Failed to start up pipeline!\n");
            }
        }
        gst_object_unref(device);
    } break;
    case GST_MESSAGE_DEVICE_REMOVED:
        gst_message_parse_device_removed(message, &device);
        name = gst_device_get_display_name(device);
        g_print("Device removed: %s\n", name);
        g_free(name);
        gst_object_unref(device);
        gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
        break;
    default:
        break;
    }

    return G_SOURCE_CONTINUE;
}

void MainWindow::create_device(const std::string &name)
{
    try {
        m_device = std::make_unique<UsbKvmDevice>(name);
        const int current_version = m_device->mcu()->get_version();
        const int expected_version = UsbKvmMcu::get_expected_version();
        if (current_version != expected_version) {
            m_mcu_info_bar_label->set_label(
                    std::format("MCU firmware update required. Need version {}, but {} is running. Video only, no "
                                "mouse or keyboard.",
                                expected_version, current_version));
            m_enter_bootloader_button->show();
            gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
            m_device->delete_mcu();
        }
        update_input_status();
    }
    catch (const std::exception &ex) {
        m_device->delete_mcu();
        m_mcu_info_bar_label->set_label("MCU not responding, may be in bootloader. Video only, no mouse or keyboard.");
        gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
    }
}

void MainWindow::enter_bootloader()
{
    if (m_device) {
        m_device->enter_bootloader();
        m_mcu_info_bar_label->set_label("In bootloader. Video only, no mouse or keyboard.");
        m_enter_bootloader_button->hide();
        gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
    }
}

MainWindow::MainWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::ApplicationWindow(cobject)
{
    {
        GstDeviceMonitor *monitor = gst_device_monitor_new();

        auto bus = gst_device_monitor_get_bus(monitor);
        gst_device_monitor_add_filter(monitor, "Video/Source", NULL);
        gst_bus_add_watch(bus, &MainWindow::monitor_bus_func, this);
        gst_object_unref(bus);
        gst_device_monitor_start(monitor);
    }

    x->get_widget("capture_resolution_box", m_capture_resolution_box);
    x->get_widget("resolution_button", m_resolution_button);
    x->get_widget("modifier_box", m_modifier_box);

    static const std::vector<std::pair<GdkModifierType, std::string>> mods = {
            {GDK_SHIFT_MASK, "Shift"},
            {GDK_CONTROL_MASK, "Ctrl"},
            {GDK_MOD1_MASK, "Alt"},
            {GDK_MOD4_MASK, "Win"},
    };

    for (const auto &[mod, name] : mods) {
        auto bu = Gtk::make_managed<Gtk::ToggleButton>(name);
        bu->show();
        m_modifier_box->pack_start(*bu, false, false, 0);
        m_modifier_buttons.emplace(mod, bu);
        bu->signal_toggled().connect([this, bu, mod] {
            auto mod_before = m_modifiers;
            if (bu->get_active())
                m_modifiers |= mod;
            else
                m_modifiers &= ~mod;
            if (mod_before != m_modifiers)
                send_keyboard_report();
        });
    }

    {
        auto rb = Gtk::make_managed<Gtk::RadioButton>("Auto");
        rb->set_active(true);
        rb->signal_toggled().connect([this, rb] {
            if (!rb->get_active())
                return;
            m_auto_capture_resolution = true;
            update_input_status();
            update_resolution_button();
        });
        rb->show();
        m_capture_resolution_box->pack_start(*rb, false, false, 0);
    }

    x->get_widget("input_status_label", m_input_status_label);
    x->get_widget("evbox", m_evbox);
    x->get_widget("mcu_info_bar", m_mcu_info_bar);
    x->get_widget("mcu_info_bar_label", m_mcu_info_bar_label);
    x->get_widget("enter_bootloader_button", m_enter_bootloader_button);
    m_enter_bootloader_button->hide();
    m_enter_bootloader_button->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::enter_bootloader));

    m_evbox->signal_realize().connect(
            [this] { m_blank_cursor = Gdk::Cursor::create(m_evbox->get_window()->get_display(), Gdk::BLANK_CURSOR); });

    Gtk::Settings::get_default()->property_gtk_menu_bar_accel() = "";
    m_evbox->signal_motion_notify_event().connect_notify(sigc::mem_fun(*this, &MainWindow::handle_motion));
    m_evbox->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &MainWindow::handle_button));
    m_evbox->signal_button_release_event().connect_notify(sigc::mem_fun(*this, &MainWindow::handle_button));
    m_evbox->signal_key_press_event().connect(sigc::mem_fun(*this, &MainWindow::handle_key));
    m_evbox->signal_key_release_event().connect(sigc::mem_fun(*this, &MainWindow::handle_key));
    m_evbox->signal_scroll_event().connect_notify(sigc::mem_fun(*this, &MainWindow::handle_scroll));
    m_evbox->signal_leave_notify_event().connect_notify([this](GdkEventCrossing *ev) {
        m_keys_pressed.clear();
        m_modifiers = 0;
        update_modifier_buttons();
        send_keyboard_report();
    });

    m_evbox->grab_focus();
    m_pipeline = gst_pipeline_new("pipeline");
#ifdef G_OS_WIN32
    m_videosrc = gst_element_factory_make("ksvideosrc", "ksvideosrc");
#else
    m_videosrc = gst_element_factory_make("v4l2src", "v4l2src");
#endif
    m_capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    m_jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    m_videocvt = gst_element_factory_make("videoconvert", "videoconvert");
    m_videosink = gst_element_factory_make("gtksink", "gtksink");
    GtkWidget *area;

    g_object_get(m_videosink, "widget", &area, NULL);
    gtk_container_add(GTK_CONTAINER(m_evbox->gobj()), area);
    gtk_widget_show(area);
    g_object_unref(area);

    gst_bin_add_many(GST_BIN(m_pipeline), m_videosrc, m_capsfilter, m_jpegdec, m_videocvt, m_videosink, NULL);

    if (!gst_element_link_many(m_videosrc, m_capsfilter, m_jpegdec, m_videocvt, m_videosink, NULL)) {
        g_warning("Failed to link videosrc to glfiltercube!\n");
        // return -1;
    }
    // set window id on this event
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_add_signal_watch_full(m_bus, G_PRIORITY_HIGH);
    g_signal_connect(m_bus, "message::error", G_CALLBACK(end_stream_cb), m_pipeline);
    g_signal_connect(m_bus, "message::warning", G_CALLBACK(end_stream_cb), m_pipeline);
    g_signal_connect(m_bus, "message::eos", G_CALLBACK(end_stream_cb), m_pipeline);
    g_signal_connect(m_bus, "message::state-changed", G_CALLBACK(state_cb), m_pipeline);
    gst_object_unref(m_bus);


    Gtk::Button *dump_button;
    x->get_widget("dump_button", dump_button);
    dump_button->signal_clicked().connect([this] {
        std::cout << m_device->mcu()->get_version() << std::endl;
        std::cout << m_device->hal().get_input_width() << "x" << m_device->hal().get_input_height() << " "
                  << m_device->hal().get_input_fps() << std::endl;
    });
    dump_button->hide();

    update_resolution_button();

    {
        auto hamburger_button = Gtk::manage(new Gtk::MenuButton);
        hamburger_button->set_image_from_icon_name("open-menu-symbolic", Gtk::ICON_SIZE_BUTTON);

        auto hamburger_menu = Gio::Menu::create();
        hamburger_button->set_menu_model(hamburger_menu);
        hamburger_button->show();
        Gtk::HeaderBar *headerbar;
        x->get_widget("headerbar", headerbar);

        headerbar->pack_end(*hamburger_button);

        hamburger_menu->append("Enter bootloader", "win.enter_bootloader");

        add_action("enter_bootloader", sigc::mem_fun(*this, &MainWindow::enter_bootloader));
    }

    m_type_window = TypeWindow::create(*this);
    m_type_window->set_transient_for(*this);
    {
        Gtk::Button *keyboard_button;
        x->get_widget("keyboard_button", keyboard_button);
        keyboard_button->signal_clicked().connect([this] { m_type_window->present(); });
    }


    Glib::signal_timeout().connect_seconds(
            [this] {
                if (!m_device)
                    return false;
                if (!m_type_window->is_busy())
                    update_input_status();
                return true;
            },
            1);
}

void MainWindow::update_input_status()
{
    if (!m_device)
        return;
    auto &hal = m_device->hal();
    std::string label;

    auto w = hal.get_input_width();
    auto h = hal.get_input_height();
    auto fps = hal.get_input_fps();
    label = format_resolution(w, h) + std::format(" {:.02f}Hz", fps);
    update_auto_capture_resolution(w, h);

    m_input_status_label->set_label(label);
}

void MainWindow::update_auto_capture_resolution(int w, int h)
{
    if (!m_auto_capture_resolution)
        return;
    if (!m_capture_resolutions.contains({w, h}))
        return;
    set_capture_resolution(w, h);
}

void MainWindow::update_resolution_button()
{
    auto [w, h] = m_capture_resolution;
    std::string label = format_resolution(w, h);
    if (m_auto_capture_resolution)
        label += " (Auto)";
    m_resolution_button->set_label(label);
}

MainWindow *MainWindow::create()
{
    MainWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/usbkvm/window.ui");
    x->get_widget_derived("mainWindow", w);
    return w;
}

UsbKvmMcu *MainWindow::get_mcu()
{
    if (m_device && m_device->mcu())
        return m_device->mcu();
    return nullptr;
}
