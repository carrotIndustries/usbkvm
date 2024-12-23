#pragma once
#include <string>
#include <vector>

namespace usbkvm {

struct DeviceInfo {
    std::string video_path;
    std::string hid_path;
    std::string bus_info;

    std::string model = "N/A";
    std::string serial = "N/A";

    using ResolutionList = std::vector<std::pair<int, int>>;
    ResolutionList capture_resolutions;
};

} // namespace usbkvm
