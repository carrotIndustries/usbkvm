#include "get_keymap.hpp"
#include <iostream>

// based on
// https://gitlab.freedesktop.org/spice/spice-gtk/-/blob/f610ac6ca5ed23a214be5b9365de91fbc5d0676f/src/vncdisplaykeymap.c

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#ifdef GDK_WINDOWING_BROADWAY
#include <gdk/gdkbroadway.h>
#endif

#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND)
/* Xorg Linux + evdev (offset evdev keycodes) */
#include "keymap_xorgevdev_usb.h"
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>
#include <stdbool.h>
#include <string.h>

/* Xorg Linux + kbd (offset + mangled XT keycodes) */
#include "keymap_xorgkbd_usb.h"
/* Xorg OS-X aka XQuartz (offset OS-X keycodes) */
#include "keymap_xorgxquartz_usb.h"
/* Xorg Cygwin aka XWin (offset + mangled XT keycodes) */
#include "keymap_xorgxwin_usb.h"

#endif

#ifdef GDK_WINDOWING_WIN32
#include <gdk/gdkwin32.h>

#include "keymap_atset1_usb.h"
#endif

#ifdef GDK_WINDOWING_QUARTZ
#include <gdk/gdkquartz.h>

/* OS-X native keycodes */
#include "keymap_osx_usb.h"
#endif

#ifdef GDK_WINDOWING_BROADWAY
/* X11 keysyms */
#include "keymap_x11_usb.h"
#endif


namespace usbkvm {

#ifdef GDK_WINDOWING_X11

#define STRPREFIX(a, b) (strncmp((a), (b), strlen((b))) == 0)

static gboolean check_for_xwin(GdkDisplay *dpy)
{
    char *vendor = ServerVendor(gdk_x11_display_get_xdisplay(dpy));

    if (strstr(vendor, "Cygwin/X"))
        return TRUE;

    return FALSE;
}

static gboolean check_for_xquartz(GdkDisplay *dpy)
{
    int nextensions;
    int i;
    gboolean match = FALSE;
    char **extensions = XListExtensions(gdk_x11_display_get_xdisplay(dpy), &nextensions);
    for (i = 0; extensions != NULL && i < nextensions; i++) {
        if (strcmp(extensions[i], "Apple-WM") == 0 || strcmp(extensions[i], "Apple-DRI") == 0)
            match = TRUE;
    }
    if (extensions)
        XFreeExtensionList(extensions);

    return match;
}

#endif

static void VNC_DEBUG(const char *x)
{
    std::cout << x << std::endl;
}

#define MAKE_RANGE(x) {(x), G_N_ELEMENTS((x))}

std::span<const uint16_t> get_keymap(GdkWindow *window)
{
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_WINDOW(window)) {
        XkbDescPtr desc;
        const gchar *keycodes = NULL;
        GdkDisplay *dpy = gdk_window_get_display(window);

        /* There is no easy way to determine what X11 server
         * and platform & keyboard driver is in use. Thus we
         * do best guess heuristics.
         *
         * This will need more work for people with other
         * X servers..... patches welcomed.
         */

        Display *xdisplay = gdk_x11_display_get_xdisplay(dpy);
        desc = XkbGetMap(xdisplay, XkbGBN_AllComponentsMask, XkbUseCoreKbd);
        if (desc) {
            if (XkbGetNames(xdisplay, XkbKeycodesNameMask, desc) == Success) {
                keycodes = gdk_x11_get_xatom_name(desc->names->keycodes);
                if (!keycodes)
                    g_warning("could not lookup keycode name");
            }
            XkbFreeKeyboard(desc, XkbGBN_AllComponentsMask, True);
        }

        if (check_for_xwin(dpy)) {
            VNC_DEBUG("Using xwin keycode mapping");
            return MAKE_RANGE(code_map_xorgxwin_to_usb);
        }
        else if (check_for_xquartz(dpy)) {
            VNC_DEBUG("Using xquartz keycode mapping");
            return MAKE_RANGE(code_map_xorgxquartz_to_usb);
        }
        else if ((keycodes && STRPREFIX(keycodes, "evdev")) || (XKeysymToKeycode(xdisplay, XK_Page_Up) == 0x70)) {
            VNC_DEBUG("Using evdev keycode mapping");
            return MAKE_RANGE(code_map_xorgevdev_to_usb);
        }
        else if ((keycodes && STRPREFIX(keycodes, "xfree86")) || (XKeysymToKeycode(xdisplay, XK_Page_Up) == 0x63)) {
            VNC_DEBUG("Using xfree86 keycode mapping");
            return MAKE_RANGE(code_map_xorgkbd_to_usb);
        }
        else {
            g_warning(
                    "Unknown keycode mapping '%s'.\n"
                    "Please report to gtk-vnc-list@gnome.org\n"
                    "including the following information:\n"
                    "\n"
                    "  - Operating system\n"
                    "  - GDK build\n"
                    "  - X11 Server\n"
                    "  - xprop -root\n"
                    "  - xdpyinfo\n",
                    keycodes);
            return {};
        }
    }
#endif

#ifdef GDK_WINDOWING_WIN32
    if (GDK_IS_WIN32_WINDOW(window)) {
        VNC_DEBUG("Using Win32 virtual keycode mapping");
        return MAKE_RANGE(code_map_atset1_to_usb);
    }
#endif

#ifdef GDK_WINDOWING_QUARTZ
    if (GDK_IS_QUARTZ_WINDOW(window)) {
        VNC_DEBUG("Using OS-X virtual keycode mapping");
        return MAKE_RANGE(code_map_osx_to_usb);
    }
#endif

#ifdef GDK_WINDOWING_WAYLAND
    if (GDK_IS_WAYLAND_WINDOW(window)) {
        VNC_DEBUG("Using Wayland Xorg/evdev virtual keycode mapping");
        return MAKE_RANGE(code_map_xorgevdev_to_usb);
    }
#endif

#ifdef GDK_WINDOWING_BROADWAY
    if (GDK_IS_BROADWAY_WINDOW(window)) {
        g_warning(
                "experimental: using broadway, x11 virtual keysym mapping - with very limited support. See also "
                "https://bugzilla.gnome.org/show_bug.cgi?id=700105");

        return MAKE_RANGE(code_map_x11_to_usb);
    }
#endif

    g_warning(
            "Unsupported GDK Windowing platform.\n"
            "Disabling extended keycode tables.\n"
            "Please report to gtk-vnc-list@gnome.org\n"
            "including the following information:\n"
            "\n"
            "  - Operating system\n"
            "  - GDK Windowing system build\n");
    return {};
}

} // namespace usbkvm
