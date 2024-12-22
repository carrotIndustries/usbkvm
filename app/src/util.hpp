#pragma once
#include <optional>
#include <string>

namespace usbkvm {
std::string wide_string_to_string(const std::wstring &wstr);
std::wstring wide_string_from_string(const std::string &str);

#ifdef G_OS_WIN32
std::optional<std::string> get_win32_hid_bus_info(const std::string &path);
std::optional<std::string> get_win32_video_bus_info(const std::string &path);
#endif

} // namespace usbkvm
