#pragma once
#include <stdint.h>
#include <span>
#include <gtk/gtk.h>

namespace usbkvm {
std::span<const uint16_t> get_keymap(GdkWindow *window);
}
