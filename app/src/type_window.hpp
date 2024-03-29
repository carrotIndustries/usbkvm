#pragma once
#include <gtkmm.h>
#include <atomic>
class IMcuProvider;

class TypeWindow : public Gtk::Window {
public:
    TypeWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IMcuProvider &mcu_provider);
    static TypeWindow *create(IMcuProvider &mcu_provider);
    bool is_busy() const
    {
        return m_is_busy;
    }

private:
    void type_text();
    IMcuProvider &m_mcu_provider;
    Glib::ustring m_text;
    void type_thread();

    Gtk::TextView *m_text_view = nullptr;
    Gtk::Button *m_type_button = nullptr;
    Gtk::Button *m_cancel_button = nullptr;
    Gtk::ProgressBar *m_progress_bar = nullptr;
    Glib::Dispatcher m_dispatcher;
    std::atomic_bool m_is_busy = false;
    std::atomic_uint32_t m_pos = false;
    std::atomic_bool m_cancel = false;
};
