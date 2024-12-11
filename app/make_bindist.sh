#!/usr/bin/env bash
DISTDIR=dist/usbkvm
BINDIR=dist/usbkvm/bin
rm -rf dist
mkdir -p $BINDIR
cp build/usbkvm.exe $BINDIR
strip $BINDIR/usbkvm.exe
LIBS=(
	libstdc++-6.dll\
	libgcc_s_seh-1.dll\
	libglibmm-2.4-1.dll\
	libglib-2.0-0.dll\
	libgio-2.0-0.dll\
	libgiomm-2.4-1.dll\
	libwinpthread-1.dll\
	libgmodule-2.0-0.dll\
	libgobject-2.0-0.dll\
	zlib1.dll\
	libintl-8.dll\
	libsigc-2.0-0.dll\
	libffi-8.dll\
	libiconv-2.dll\
	libpcre2-8-0.dll\
	libatkmm-1.6-1.dll\
	libatk-1.0-0.dll\
	libgtk-3-0.dll\
	libgtkmm-3.0-1.dll\
	libpango-1.0-0.dll\
	libpangomm-1.4-1.dll\
	libcairomm-1.0-1.dll\
	libcairo-2.dll\
	libpangocairo-1.0-0.dll\
	libgdk-3-0.dll\
	libgdkmm-3.0-1.dll\
	libgdk_pixbuf-2.0-0.dll\
	libpangoft2-1.0-0.dll\
	libpangowin32-1.0-0.dll\
	libfontconfig-1.dll\
	libfreetype-6.dll\
	libcairo-gobject-2.dll\
	libepoxy-0.dll\
	libharfbuzz-0.dll\
	libpixman-1-0.dll\
	libpng16-16.dll\
	libexpat-1.dll\
	libbz2-1.dll\
	libgraphite2.dll\
	libjpeg-8.dll\
	librsvg-2-2.dll\
	libxml2-2.dll\
	liblzma-5.dll\
	libtiff-6.dll\
	libbrotlicommon.dll\
	libbrotlidec.dll\
	libfribidi-0.dll\
	libthai-0.dll\
	libdatrie-1.dll\
	libzstd.dll\
	libdeflate.dll\
	libwebp-7.dll\
	libjbig-0.dll\
	libLerc.dll\
	libsharpyuv-0.dll\
	gspawn-win64-helper.exe\
	gspawn-win64-helper-console.exe\
	gst-device-monitor-1.0.exe\
	libhidapi-0.dll\
	libgstreamer-1.0-0.dll\
	libgstvideo-1.0-0.dll\
	libgstbase-1.0-0.dll\
	liborc-0.4-0.dll\
)
for LIB in "${LIBS[@]}"
do
   cp /mingw64/bin/$LIB $BINDIR
done

mkdir -p $DISTDIR/share/icons
cp -r /mingw64/share/icons/Adwaita $DISTDIR/share/icons
cp -r /mingw64/share/icons/hicolor $DISTDIR/share/icons
rm -rf $DISTDIR/share/icons/Adwaita/cursors

mkdir -p $DISTDIR/lib
cp -r /mingw64/lib/gdk-pixbuf-2.0 $DISTDIR/lib
gdk-pixbuf-query-loaders > $DISTDIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
rm $DISTDIR/lib/gdk-pixbuf-*/*/loaders/*.a

mkdir -p $DISTDIR/share/glib-2.0/schemas
cp /mingw64/share/glib-2.0/schemas/gschemas.compiled $DISTDIR/share/glib-2.0/schemas

mkdir -p $DISTDIR/lib/gstreamer-1.0
cp /mingw64/lib/gstreamer-1.0/libgstgtk.dll $DISTDIR/lib/gstreamer-1.0
cp /mingw64/lib/gstreamer-1.0/libgstwinks.dll $DISTDIR/lib/gstreamer-1.0
cp /mingw64/lib/gstreamer-1.0/libgstjpeg.dll $DISTDIR/lib/gstreamer-1.0
cp /mingw64/lib/gstreamer-1.0/libgstcoreelements.dll $DISTDIR/lib/gstreamer-1.0
cp /mingw64/lib/gstreamer-1.0/libgstvideoconvertscale.dll $DISTDIR/lib/gstreamer-1.0

git log -10 | unix2dos > dist/log.txt
if [ "$1" != "-n" ]; then
	cd dist
	zip -r usbkvm-$(date +%Y-%m-%d-%H%M).zip usbkvm log.txt
fi
