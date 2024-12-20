USBKVM client app
=================

# Installation

## Arch Linux

Install the [usbkvm AUR package](https://aur.archlinux.org/packages/usbkvm).

## Windows

Grab a zip archive from the releases section, extract it and start usbkvm.exe from the `bin` directory.

## mac OS

Currently, USBKVM hasn't been tested on mac OS. You can try building it using homebrew. This may or may not work.

# Building from source

Make sure to clone the repo recursively to get all required submodules. Or get a tarball from the releases section. Take care to use the file named `usbkvm-vx.x.x.tar.gz` and not the GitHub-provided source archives as these don't include the submodules, pre-built firmware images and vendored go dependencies.

To convince yourself of the provenance of these tarballs, the action creating them prints the SHA256 hash.

## Prerequisites

Copy the udev rules `70-usbkvm.rules` to `/etc/udev/rules.d/` so that regular users can access the USBKVM HID device.

You need these dependencies:

 - gstreamer
 - gtkmm 3.0
 - meson
 - gst-plugin-gtk
 - gst-plugins-good
 - hidapi
 - go >= 1.20
 
### Building

Only when building from the repo, you first have to build the firmware in `fw/usbkvm` with `make`. See the project [README](../README.md#firmware) for more details.

Then run:
```
meson setup build
meson compile -C build
```

### Running

Invoke `build/usbkvm`