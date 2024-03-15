#pragma once
#include <gtkmm.h>
#include <gst/gst.h>

class UsbKvmDevice;

class MainWindow : public Gtk::ApplicationWindow {
public:
    MainWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static MainWindow *create();

private:
    GstElement *m_pipeline = nullptr;
    GstElement *m_videosrc = nullptr;
    GstElement *m_capsfilter = nullptr;
    GstElement *m_jpegdec = nullptr;
    GstElement *m_videocvt = nullptr;
    GstElement *m_videosink = nullptr;
    GstStateChangeReturn ret;
    GstBus *m_bus;

    Gtk::EventBox *m_evbox;
    Glib::RefPtr<Gdk::Cursor> m_blank_cursor;
    Gtk::InfoBar *m_mcu_info_bar;
    Gtk::Label *m_mcu_info_bar_label;
    Gtk::Button *m_enter_bootloader_button;
    Gtk::Box *m_modifier_box;
    std::map<GdkModifierType, Gtk::ToggleButton*> m_modifier_buttons;
    void update_modifier_buttons();

    Gtk::Label *m_input_status_label;
    void update_input_status();

    std::unique_ptr<UsbKvmDevice> m_device;
    std::set<guint> m_keys_pressed;
    guint m_modifiers = 0;
    uint32_t m_buttons_pressed = 0;
    double m_cursor_x = 0;
    double m_cursor_y = 0;

    void send_mouse_report(int vscroll_delta = 0);
    void send_keyboard_report();

    void handle_button(GdkEventButton *ev);
    bool handle_key(GdkEventKey *ev);
    void handle_scroll(GdkEventScroll *ev);
    void handle_motion(GdkEventMotion *ev);

    void create_device(const std::string &name);
    void enter_bootloader();

    std::pair<int, int> m_capture_resolution = {1280, 720};
    std::set<std::pair<int, int>> m_capture_resolutions;
    bool m_auto_capture_resolution = true;
    void update_auto_capture_resolution(int w, int h);
    void set_capture_resolution(int w, int h);

    Gtk::Button *m_resolution_button = nullptr;
    Gtk::Box *m_capture_resolution_box = nullptr;
    void update_resolution_button();

    gboolean monitor_bus_func(GstBus *bus, GstMessage *message);
    static gboolean monitor_bus_func(GstBus *bus, GstMessage *message, gpointer user_data);

    static gboolean handle_cap(GstCapsFeatures *features, GstStructure *structure, gpointer user_data);
    gboolean handle_cap(GstCapsFeatures *features, GstStructure *structure);
};

