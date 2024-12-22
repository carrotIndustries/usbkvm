#include "usbkvm_application.hpp"
#include <gst/gst.h>


int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    auto application = usbkvm::UsbKvmApplication::create();

    auto rc = application->run(argc, argv);

    return rc;
}
