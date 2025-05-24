#pragma once
#include <sigc++/sigc++.h>
#include <vector>
#include <string>

namespace usbkvm {

struct DeviceInfo;

class IDevicesProvider {
public:
    virtual std::vector<const DeviceInfo *> get_devices() const = 0;
    virtual void activate_device(const std::string &video_path) = 0;
    virtual void recover_eeprom() = 0;

    using type_signal_devices_changed = sigc::signal<void()>;
    virtual type_signal_devices_changed signal_devices_changed() = 0;
};
} // namespace usbkvm
