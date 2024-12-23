#include <glib.h>
#include "util.hpp"
#include <codecvt>
#include <locale>
#include <optional>
#include <ranges>
#include <algorithm>
#ifdef G_OS_WIN32
#include <windows.h>
#include <SetupAPI.h>
#include <initguid.h> // Put this in to get rid of linker errors.
#include <Devpkey.h>
#endif

namespace usbkvm {

std::string wide_string_to_string(const std::wstring &wstr)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.to_bytes(wstr);
}

std::wstring wide_string_from_string(const std::string &str)
{
    using convert_typeX = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_typeX, wchar_t> converterX;

    return converterX.from_bytes(str);
}

#ifdef G_OS_WIN32

static std::optional<std::wstring> GetDeviceStringProperty(HDEVINFO dev_info, SP_DEVINFO_DATA *dev_info_data,
                                                           const DEVPROPKEY &property)
{
    DEVPROPTYPE property_type;
    DWORD required_size;
    if (SetupDiGetDevicePropertyW(dev_info, dev_info_data, &property, &property_type, nullptr, 0, &required_size, 0))
        return {};

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        return {};

    if (property_type != DEVPROP_TYPE_STRING)
        return {};

    std::wstring buffer;
    buffer.resize(required_size / 2);
    if (!SetupDiGetDevicePropertyW(dev_info, dev_info_data, &property, &property_type,
                                   reinterpret_cast<PBYTE>(buffer.data()), required_size, nullptr, 0))
        return {};

    buffer = std::wstring{buffer.c_str()}; // trim to last null byte

    return buffer;
}

static std::optional<std::wstring> get_device_parent(const std::wstring &path)
{
    HDEVINFO hDevInfoSet;
    SP_DEVINFO_DATA devInfoData;
    hDevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (hDevInfoSet == INVALID_HANDLE_VALUE)
        return {};

    if (!SetupDiOpenDeviceInterfaceW(hDevInfoSet, path.c_str(), 0, NULL)) {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
        return {};
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiEnumDeviceInfo(hDevInfoSet, 0, &devInfoData)) {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
        return {};
    }

    auto ret = GetDeviceStringProperty(hDevInfoSet, &devInfoData, DEVPKEY_Device_Parent);
    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    return ret;
}

static std::optional<std::wstring> get_device_parent_from_instance(const std::wstring &instance)
{
    HDEVINFO hDevInfoSet;
    SP_DEVINFO_DATA devInfoData;
    hDevInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);

    if (hDevInfoSet == INVALID_HANDLE_VALUE)
        return {};

    if(!SetupDiOpenDeviceInfoW(hDevInfoSet, instance.c_str(), NULL, 0, NULL)) {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
        return {};
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiEnumDeviceInfo(hDevInfoSet, 0, &devInfoData)) {
        SetupDiDestroyDeviceInfoList(hDevInfoSet);
        return {};
    }

    auto ret = GetDeviceStringProperty(hDevInfoSet, &devInfoData, DEVPKEY_Device_Parent);
    SetupDiDestroyDeviceInfoList(hDevInfoSet);

    return ret;
}

std::optional<std::string> get_win32_hid_bus_info(const std::string &path)
{
    auto parent = get_device_parent(wide_string_from_string(path));
    if (!parent)
        return {};
    auto parent2 = get_device_parent_from_instance(parent.value());
    if (!parent2)
        return {};
    return wide_string_to_string(parent2.value());
}

std::optional<std::string> get_win32_video_bus_info(const std::string &path)
{
    auto parent = get_device_parent(wide_string_from_string(path));
    if (!parent)
        return {};
    return wide_string_to_string(parent.value());
}


#endif

} // namespace usbkvm
