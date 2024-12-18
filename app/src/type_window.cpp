#include "type_window.hpp"
#include "usbkvm_mcu.hpp"
#include "imcu_provider.hpp"
#include "mshal.hpp"
#include <thread>

struct KeyItem {
    uint8_t scancode;
    UsbKvmMcu::KeyboardReport::Modifier mod;
};

static const std::array s_scancodes = {
        /* 0x00 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x01 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x02 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x03 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x04 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x05 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x06 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x07 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x08 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x09 ' ' */ KeyItem{0x2b, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0A ' ' */ KeyItem{0x28, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0B ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0C ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0D ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0E ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x0F ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x10 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x11 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x12 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x13 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x14 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x15 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x16 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x17 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x18 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x19 ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1A ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1B ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1C ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1D ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1E ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x1F ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x20 ' ' */ KeyItem{0x2c, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x21 '!' */ KeyItem{0x1e, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x22 '"' */ KeyItem{0x34, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x23 '#' */ KeyItem{0x20, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x24 '$' */ KeyItem{0x21, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x25 '%' */ KeyItem{0x22, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x26 '&' */ KeyItem{0x24, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x27 ''' */ KeyItem{0x34, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x28 '(' */ KeyItem{0x26, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x29 ')' */ KeyItem{0x27, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x2A '*' */ KeyItem{0x25, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x2B '+' */ KeyItem{0x2e, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x2C ',' */ KeyItem{0x36, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x2D '-' */ KeyItem{0x2d, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x2E '.' */ KeyItem{0x37, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x2F '/' */ KeyItem{0x38, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x30 '0' */ KeyItem{0x27, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x31 '1' */ KeyItem{0x1e, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x32 '2' */ KeyItem{0x1f, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x33 '3' */ KeyItem{0x20, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x34 '4' */ KeyItem{0x21, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x35 '5' */ KeyItem{0x22, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x36 '6' */ KeyItem{0x23, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x37 '7' */ KeyItem{0x24, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x38 '8' */ KeyItem{0x25, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x39 '9' */ KeyItem{0x26, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x3A ':' */ KeyItem{0x33, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x3B ';' */ KeyItem{0x33, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x3C '<' */ KeyItem{0x36, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x3D '=' */ KeyItem{0x2e, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x3E '>' */ KeyItem{0x37, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x3F '?' */ KeyItem{0x38, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x40 '@' */ KeyItem{0x1f, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x41 'A' */ KeyItem{0x04, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x42 'B' */ KeyItem{0x05, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x43 'C' */ KeyItem{0x06, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x44 'D' */ KeyItem{0x07, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x45 'E' */ KeyItem{0x08, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x46 'F' */ KeyItem{0x09, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x47 'G' */ KeyItem{0x0a, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x48 'H' */ KeyItem{0x0b, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x49 'I' */ KeyItem{0x0c, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4A 'J' */ KeyItem{0x0d, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4B 'K' */ KeyItem{0x0e, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4C 'L' */ KeyItem{0x0f, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4D 'M' */ KeyItem{0x10, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4E 'N' */ KeyItem{0x11, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x4F 'O' */ KeyItem{0x12, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x50 'P' */ KeyItem{0x13, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x51 'Q' */ KeyItem{0x14, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x52 'R' */ KeyItem{0x15, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x53 'S' */ KeyItem{0x16, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x54 'T' */ KeyItem{0x17, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x55 'U' */ KeyItem{0x18, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x56 'V' */ KeyItem{0x19, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x57 'W' */ KeyItem{0x1a, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x58 'X' */ KeyItem{0x1b, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x59 'Y' */ KeyItem{0x1c, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x5A 'Z' */ KeyItem{0x1d, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x5B '[' */ KeyItem{0x2f, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x5C '\' */ KeyItem{0x32, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x5D ']' */ KeyItem{0x30, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x5E '^' */ KeyItem{0x23, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x5F '_' */ KeyItem{0x2d, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x60 '`' */ KeyItem{0x34, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x61 'a' */ KeyItem{0x04, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x62 'b' */ KeyItem{0x05, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x63 'c' */ KeyItem{0x06, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x64 'd' */ KeyItem{0x07, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x65 'e' */ KeyItem{0x08, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x66 'f' */ KeyItem{0x09, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x67 'g' */ KeyItem{0x0a, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x68 'h' */ KeyItem{0x0b, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x69 'i' */ KeyItem{0x0c, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6A 'j' */ KeyItem{0x0d, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6B 'k' */ KeyItem{0x0e, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6C 'l' */ KeyItem{0x0f, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6D 'm' */ KeyItem{0x10, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6E 'n' */ KeyItem{0x11, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x6F 'o' */ KeyItem{0x12, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x70 'p' */ KeyItem{0x13, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x71 'q' */ KeyItem{0x14, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x72 'r' */ KeyItem{0x15, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x73 's' */ KeyItem{0x16, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x74 't' */ KeyItem{0x17, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x75 'u' */ KeyItem{0x18, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x76 'v' */ KeyItem{0x19, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x77 'w' */ KeyItem{0x1a, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x78 'x' */ KeyItem{0x1b, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x79 'y' */ KeyItem{0x1c, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x7A 'z' */ KeyItem{0x1d, UsbKvmMcu::KeyboardReport::Modifier::NONE},
        /* 0x7B '{' */ KeyItem{0x2f, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x7C '|' */ KeyItem{0x32, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x7D '}' */ KeyItem{0x30, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x7E '~' */ KeyItem{0x35, UsbKvmMcu::KeyboardReport::Modifier::LEFTSHIFT},
        /* 0x7F ' ' */ KeyItem{0x00, UsbKvmMcu::KeyboardReport::Modifier::NONE},
};

static_assert(s_scancodes.size() == 128);

TypeWindow::TypeWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, IMcuProvider &mcu_provider)
    : Gtk::Window(cobject), m_mcu_provider(mcu_provider)
{
    x->get_widget("type_button", m_type_button);
    x->get_widget("cancel_button", m_cancel_button);
    m_type_button->signal_clicked().connect(sigc::mem_fun(*this, &TypeWindow::type_text));
    x->get_widget("text_view", m_text_view);
    x->get_widget("progress_bar", m_progress_bar);
    m_cancel_button->hide();

    m_dispatcher.connect([this] {
        if (m_is_busy == false) {
            m_progress_bar->set_fraction(0);
            m_type_button->set_sensitive(true);
            set_modal(false);
            m_cancel_button->hide();
            std::string io_error;
            {
                std::lock_guard<std::mutex> guard{m_io_error_mutex};
                io_error = m_io_error;
            }
            if (io_error.size())
                m_mcu_provider.handle_io_error(io_error);
        }
        else {
            m_progress_bar->set_fraction((double)m_pos / m_text.size());
        }
    });

    m_cancel_button->signal_clicked().connect([this] { m_cancel = true; });
}

void TypeWindow::type_text()
{
    if (!m_mcu_provider.get_mcu())
        return;

    if (m_is_busy)
        return;

    m_is_busy = true;
    m_type_button->set_sensitive(false);
    m_pos = 0;
    m_progress_bar->set_fraction(0);
    m_cancel_button->show();
    m_cancel = false;
    m_io_error = "";
    set_modal(true);
    m_text = m_text_view->get_buffer()->get_text();
    auto thr = std::thread(&TypeWindow::type_thread, this);
    thr.detach();
}

void TypeWindow::type_thread()
{
    auto &mcu = *m_mcu_provider.get_mcu();
    for (const auto chr : m_text) {
        if (chr >= s_scancodes.size())
            continue;
        if (m_cancel)
            break;
        auto &ki = s_scancodes.at(chr);
        UsbKvmMcu::KeyboardReport report;
        report.keycode.at(0) = ki.scancode;
        report.mod = ki.mod;
        try {
            if (!mcu.send_report(report))
                break;
            if (!mcu.send_report(UsbKvmMcu::KeyboardReport{}))
                break;
        }
        catch (MsHal::IOError &err) {
            {
                std::lock_guard<std::mutex> guard{m_io_error_mutex};
                m_io_error = err.what();
            }
            break;
        }
        m_pos++;
        m_dispatcher.emit();
    }
    m_is_busy = false;
    m_dispatcher.emit();
}

TypeWindow *TypeWindow::create(IMcuProvider &mcu_provider)
{
    TypeWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/net/carrotIndustries/usbkvm/type_window.ui");
    x->get_widget_derived("window", w, mcu_provider);
    return w;
}
