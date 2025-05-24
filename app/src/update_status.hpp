#pragma once
#include <string>

namespace usbkvm {

enum class UpdateStatus { BUSY, DONE, ERROR };

struct UpdateProgress {
    UpdateStatus status;
    std::string message;
    float progress = 0;
};
} // namespace usbkvm
