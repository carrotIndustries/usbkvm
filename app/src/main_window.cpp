#include "main_window.hpp"
#include <cassert>
#include <iostream>
#include "keymap.h"
#include <format>
#include <thread>
#include "usbkvm_mcu.hpp"
#include "usbkvm_device.hpp"
#include "mshal.hpp"
#include "type_window.hpp"
#include "about_dialog.hpp"

namespace usbkvm {

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


void UsbKvmAppWindow::handle_button(GdkEventButton *ev)
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

void UsbKvmAppWindow::send_mouse_report(int scroll_delta)
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

    try {
        mcu.send_report(report);
    }
    catch (MsHal::IOError &e) {
        handle_io_error(e.what());
    }
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

void UsbKvmAppWindow::send_keyboard_report()
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
    try {
        mcu.send_report(report);
    }
    catch (MsHal::IOError &e) {
        handle_io_error(e.what());
    }
}


void UsbKvmAppWindow::handle_scroll(GdkEventScroll *ev)
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


bool UsbKvmAppWindow::handle_key(GdkEventKey *ev)
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

void UsbKvmAppWindow::handle_motion(GdkEventMotion *ev)
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

void UsbKvmAppWindow::update_modifier_buttons()
{
    for (auto &[mod, bu] : m_modifier_buttons) {
        bu->set_active(m_modifiers & mod);
    }
}

gboolean UsbKvmAppWindow::monitor_bus_func(GstBus *bus, GstMessage *message, gpointer pself)
{
    auto self = reinterpret_cast<UsbKvmAppWindow *>(pself);
    return self->monitor_bus_func(bus, message);
}

gboolean UsbKvmAppWindow::handle_cap(GstCapsFeatures *features, GstStructure *structure, gpointer user_data)
{
    auto self = reinterpret_cast<UsbKvmAppWindow *>(user_data);
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

gboolean UsbKvmAppWindow::handle_cap(GstCapsFeatures *features, GstStructure *structure)
{
    std::string name = gst_structure_get_name(structure);
    if (name == "image/jpeg") {
        gint width, height;
        if (gst_structure_get_int(structure, "width", &width) && gst_structure_get_int(structure, "height", &height)) {
            if (!m_capture_resolutions.emplace(width, height).second)
                return true;
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

void UsbKvmAppWindow::set_capture_resolution(int w, int h)
{
    if (m_capture_resolution == std::make_pair(w, h))
        return;
    m_capture_resolution = {w, h};
    auto caps = gst_caps_new_simple("image/jpeg", "width", G_TYPE_INT, w, "height", G_TYPE_INT, h, NULL);
    g_object_set(m_capsfilter, "caps", caps, NULL);
    gst_caps_unref(caps);
    update_resolution_button();
}

static const std::string s_device_name = "USBKVM";

gboolean UsbKvmAppWindow::monitor_bus_func(GstBus *bus, GstMessage *message)
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
        if (name_str.starts_with(s_device_name)) {
            std::cout << "found usbkvm" << std::endl;
            set_overlay_label_text("");
            {
                auto caps = gst_device_get_caps(device);
                gst_caps_foreach(caps, &UsbKvmAppWindow::handle_cap, this);
                gst_caps_unref(caps);
            }
            if (m_pipeline) {
#ifdef G_OS_WIN32
                g_object_set(m_videosrc, "device-name", s_device_name.c_str(), NULL);
#else
                {
                    auto props = gst_device_get_properties(device);
                    auto path = gst_structure_get_string(props, "api.v4l2.path");

                    // v4l2.device.bus_info
                    // api.v4l2.cap.bus_info

                    if (!path)
                        path = gst_structure_get_string(props, "device.path");

                    if (path)
                        g_object_set(m_videosrc, "device", path, NULL);


                    gst_structure_free(props);
                }
#endif
                auto ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
                if (ret == GST_STATE_CHANGE_FAILURE) {
                    g_print("Failed to start up pipeline!\n");
                }
                else {
                    m_has_video = true;
                }
            }
            create_device(s_device_name);
        }
        gst_object_unref(device);
    } break;
    case GST_MESSAGE_DEVICE_REMOVED: {
        gst_message_parse_device_removed(message, &device);
        name = gst_device_get_display_name(device);
        std::string name_s = name;
        g_print("Device removed: %s\n", name);
        g_free(name);
        gst_object_unref(device);
        if (name_s == s_device_name) {
            gst_element_set_state(m_pipeline, GST_STATE_NULL);
            m_has_video = false;
            set_overlay_label_text("No USBKVM connected");
        }
    } break;
    default:
        break;
    }

    return G_SOURCE_CONTINUE;
}

void UsbKvmAppWindow::create_device(const std::string &name)
{
    if (m_device)
        return;
    try {
        m_device = std::make_unique<UsbKvmDevice>(name);
        auto info = m_device->mcu()->get_info();
        const int current_version = info.version;
        m_device->set_model(info.model);
        const int expected_version = UsbKvmMcu::get_expected_version();
        if (current_version != expected_version) {
            std::string l;

            switch (info.get_valid()) {
                using enum UsbKvmMcu::Info::Valid;
            case VALID:
                l = std::format("Need version {}, but {} is running.", expected_version, current_version);
                break;

            case INVALID_MAGIC:
                l = "Firmware corrupted: Invalid magic number";
                break;

            case APP_CRC_MISMATCH:
                l = "Firmware corrupted: App CRC mismatch";
                break;

            case HEADER_CRC_MISMATCH:
                l = "Firmware corrupted: Header CRC mismatch";
                break;
            }
            m_mcu_info_bar_label->set_label(
                    std::format("MCU firmware update required. {} Video only, no mouse or keyboard.", l));
            m_device->enter_bootloader();
            m_update_firmware_button->show();
            gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
        }
        else {
            if (info.in_bootloader) {
                m_device->mcu()->boot_start_app();
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);

                auto info2 = m_device->mcu()->get_info();
                if (info2.in_bootloader) {
                    m_mcu_info_bar_label->set_label("MCU stuck in bootloader. Video only, no mouse or keyboard.");
                    gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
                    m_device->delete_mcu();
                }
                if (info2.version != info.version) {
                    m_mcu_info_bar_label->set_label(
                            "MCU app version not as expected. Video only, no mouse or keyboard.");
                    gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
                    m_device->delete_mcu();
                }
            }
        }
    }
    catch (const std::exception &ex) {
        if (m_device)
            m_device->delete_mcu();
        m_mcu_info_bar_label->set_label("MCU not responding. Video only, no mouse or keyboard.");
        gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), true);
    }

    if (m_device) {
        update_input_status();
        switch (m_device->get_model()) {
        case Model::USBKVM:
            set_title("USBKVM");
            break;
        case Model::USBKVM_PRO:
            set_title("USBKVM Pro");
            break;
        default:;
        }

        Glib::signal_timeout().connect_seconds(
                [this] {
                    if (!m_device)
                        return false;
                    if (!m_type_window->is_busy())
                        return update_input_status();
                    return true;
                },
                1);
    }
}

void UsbKvmAppWindow::update_firmware()
{
    if (m_device) {
        gtk_info_bar_set_revealed(m_mcu_info_bar->gobj(), false);
        m_firmware_update_revealer->set_reveal_child(true);
        auto mcu = m_device->mcu_boot();
        if (!mcu) {
            m_firmware_update_label->set_label("Not in bootloader mode");
            return;
        }
        m_firmware_update_status = FirmwareUpdateStatus::BUSY;
        m_firmware_update_progress = 0;
        m_firmware_update_status_string.clear();
        m_firmware_update_label->set_label("Intializing");

        auto thr = std::thread(&UsbKvmAppWindow::firmware_update_thread, this);
        thr.detach();
    }
}

void UsbKvmAppWindow::firmware_update_thread()
{

    auto update_status = [this](FirmwareUpdateStatus status, const std::string &msg) {
        {
            std::lock_guard<std::mutex> guard(m_firmware_update_status_mutex);
            if (status == FirmwareUpdateStatus::ERROR)
                m_firmware_update_status_string = "Error: " + msg;
            else
                m_firmware_update_status_string = msg;
        }
        m_firmware_update_status = status;
        m_firmware_update_dispatcher.emit();
    };

    try {
        auto mcu = m_device->mcu_boot();
        auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/usbkvm/usbkvm.bin");
        if (!bytes)
            throw std::runtime_error("can't open firmware blob");

        gsize data_size;
        auto data = reinterpret_cast<const uint8_t *>(bytes->get_data(data_size));

        const auto update_result = mcu->boot_update_firmware(
                [this, &update_status](const UsbKvmMcu::FirmwareUpdateProgress &status) {
                    if (status.status == FirmwareUpdateStatus::BUSY)
                        m_firmware_update_progress = status.progress;
                    update_status(status.status, status.message);
                },
                {data, data_size});

        if (update_result) {
            m_device->leave_bootloader();
            using namespace std::chrono_literals;

            std::this_thread::sleep_for(700ms);
            update_status(FirmwareUpdateStatus::DONE, "Done");
        }
    }
    catch (const std::exception &ex) {
        update_status(FirmwareUpdateStatus::ERROR, std::string{"Error: "} + ex.what());
    }
    catch (...) {
        update_status(FirmwareUpdateStatus::ERROR, "Unknown error");
    }
}

void UsbKvmAppWindow::update_firmware_update_status()
{
    std::string status;
    {
        std::lock_guard<std::mutex> guard(m_firmware_update_status_mutex);
        status = m_firmware_update_status_string;
    }
    m_firmware_update_label->set_label("Firmware update: " + status);
    m_firmware_update_progress_bar->set_fraction(m_firmware_update_progress);
    m_firmware_update_revealer->set_reveal_child(m_firmware_update_status != FirmwareUpdateStatus::DONE);
}

UsbKvmAppWindow::UsbKvmAppWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x) : Gtk::ApplicationWindow(cobject)
{
    {
        GstDeviceMonitor *monitor = gst_device_monitor_new();

        auto bus = gst_device_monitor_get_bus(monitor);
        gst_device_monitor_add_filter(monitor, "Video/Source", NULL);
        gst_bus_add_watch(bus, &UsbKvmAppWindow::monitor_bus_func, this);
        gst_object_unref(bus);
        gst_device_monitor_start(monitor);
    }

    set_icon_name("usbkvm");

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
    x->get_widget("overlay_label", m_overlay_label);
    x->get_widget("mcu_info_bar", m_mcu_info_bar);
    x->get_widget("mcu_info_bar_label", m_mcu_info_bar_label);
    x->get_widget("update_firmware_button", m_update_firmware_button);
    x->get_widget("firmware_update_revealer", m_firmware_update_revealer);
    x->get_widget("firmware_update_label", m_firmware_update_label);
    x->get_widget("firmware_update_progress_bar", m_firmware_update_progress_bar);
    m_update_firmware_button->hide();
    m_update_firmware_button->signal_clicked().connect(sigc::mem_fun(*this, &UsbKvmAppWindow::update_firmware));
    m_firmware_update_dispatcher.connect(sigc::mem_fun(*this, &UsbKvmAppWindow::update_firmware_update_status));
    m_firmware_update_status = FirmwareUpdateStatus::DONE;

    m_evbox->signal_realize().connect(
            [this] { m_blank_cursor = Gdk::Cursor::create(m_evbox->get_window()->get_display(), Gdk::BLANK_CURSOR); });

    m_evbox->signal_motion_notify_event().connect_notify(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_motion));
    m_evbox->signal_button_press_event().connect_notify(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_button));
    m_evbox->signal_button_release_event().connect_notify(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_button));
    m_evbox->signal_key_press_event().connect(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_key));
    m_evbox->signal_key_release_event().connect(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_key));
    m_evbox->signal_scroll_event().connect_notify(sigc::mem_fun(*this, &UsbKvmAppWindow::handle_scroll));
    m_evbox->signal_leave_notify_event().connect_notify([this](GdkEventCrossing *ev) {
        m_keys_pressed.clear();
        m_modifiers = 0;
        update_modifier_buttons();
        send_keyboard_report();
    });

    m_evbox->grab_focus();

#ifdef G_OS_WIN32
    m_videosrc = gst_element_factory_make("ksvideosrc", "ksvideosrc");
#else
    m_videosrc = gst_element_factory_make("v4l2src", "v4l2src");
#endif
    m_capsfilter = gst_element_factory_make("capsfilter", "capsfilter");
    m_jpegdec = gst_element_factory_make("jpegdec", "jpegdec");
    m_videocvt = gst_element_factory_make("videoconvert", "videoconvert");
    m_videosink = gst_element_factory_make("gtksink", "gtksink");

    std::string gst_errors;
    if (m_videosrc == nullptr) {
        gst_errors += "no videosrc, ";
    }
    if (m_capsfilter == nullptr) {
        gst_errors += "no capsfilter, ";
    }
    if (m_jpegdec == nullptr) {
        gst_errors += "no jpegdec, ";
    }
    if (m_videocvt == nullptr) {
        gst_errors += "no videoconvert, ";
    }
    if (m_videosink == nullptr) {
        gst_errors += "no videosink, ";
    }
    if (gst_errors.size() >= 2) {
        gst_errors.pop_back();
        gst_errors.pop_back();

        auto error_label = Gtk::make_managed<Gtk::Label>("couldn't initialize gstreamer: " + gst_errors);
        error_label->set_line_wrap(true);
        error_label->show();
        m_evbox->add(*error_label);
    }
    else {
        m_pipeline = gst_pipeline_new("pipeline");

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
    }


    Gtk::Button *dump_button;
    x->get_widget("dump_button", dump_button);
    dump_button->signal_clicked().connect([this] {
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

        hamburger_menu->append("About", "win.about");

        add_action("about", [this] {
            AboutDialog dia;
            dia.set_transient_for(*this);
            dia.run();
        });
    }

    m_type_window = TypeWindow::create(*this);
    m_type_window->set_transient_for(*this);
    {
        Gtk::Button *keyboard_button;
        x->get_widget("keyboard_button", keyboard_button);
        keyboard_button->signal_clicked().connect([this] { m_type_window->present(); });
    }

    set_overlay_label_text("No USBKVM connected");
}

bool UsbKvmAppWindow::update_input_status()
{
    if (!m_device) {
        m_input_status_label->set_label("Not connected");
        return false;
    }
    if (m_firmware_update_status != FirmwareUpdateStatus::DONE)
        return true;
    try {
        auto &hal = m_device->hal();
        std::string label;

        if (hal.get_has_signal()) {
            auto w = hal.get_input_width();
            auto h = hal.get_input_height();
            auto fps = hal.get_input_fps();
            label += format_resolution(w, h) + std::format("  {:.01f}Hz", fps);
            update_auto_capture_resolution(w, h);

            if (m_device->get_model() == Model::USBKVM_PRO) {
                if (auto mcu = m_device->mcu()) {
                    auto status = mcu->get_status();
                    if (status.vga_connected)
                        label += " VGA";
                    else
                        label += " HDMI";
                }
            }
            set_overlay_label_text("");
        }
        else {
            label = "No signal";
            set_overlay_label_text(label);
        }
        m_input_status_label->set_label(label);
    }
    catch (MsHal::IOError &e) {
        handle_io_error(e.what());
        return false;
    }
    return true;
}

void UsbKvmAppWindow::update_auto_capture_resolution(int w, int h)
{
    if (!m_auto_capture_resolution)
        return;
    if (!m_capture_resolutions.contains({w, h}))
        return;
    set_capture_resolution(w, h);
}

void UsbKvmAppWindow::update_resolution_button()
{
    auto [w, h] = m_capture_resolution;
    std::string label = format_resolution(w, h);
    if (m_auto_capture_resolution)
        label += " (Auto)";
    m_resolution_button->set_label(label);
}

UsbKvmAppWindow *UsbKvmAppWindow::create()
{
    UsbKvmAppWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/usbkvm/window.ui");
    x->get_widget_derived("mainWindow", w);
    return w;
}

UsbKvmMcu *UsbKvmAppWindow::get_mcu()
{
    if (m_device && m_device->mcu())
        return m_device->mcu();
    return nullptr;
}

void UsbKvmAppWindow::set_overlay_label_text(const std::string &label)
{
    m_overlay_label->set_visible(label.size());
    m_overlay_label->set_label(label);
}

void UsbKvmAppWindow::handle_io_error(const std::string &err)
{
    m_device.reset();
    m_input_status_label->set_label("IO Error: " + err);
    if (m_has_video) {
        // video device there, try reconnecting
        Glib::signal_timeout().connect_seconds_once(
                [this] {
                    if (m_has_video) {
                        create_device(s_device_name);
                    }
                },
                1);
    }
}

} // namespace usbkvm
