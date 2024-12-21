#include "version.hpp"

namespace usbkvm {

std::string Version::get_string()
{
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(micro);
}

} // namespace usbkvm
