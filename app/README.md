USBKVM client app
=================

# Prerequisites

Copy the udev rules `80-usbkvm.rules` to `/etc/udev/rules.d/` so that regular
users can access the USBKVM HID device.

You need these dependencies:

 - gstreamer
 - gtkmm 3.0
 - meson
 - gst-plugin-gtk
 - gst-plugins-good
 - hidapi
 - go >= 1.20
 
# Building

Make sure to clone the repo recursively to get the required submodules.
Then run
```
meson setup build
meson compile -C build
```

# Running

Invoke `build/usbkvm`
