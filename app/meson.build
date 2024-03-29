project('usbkvm', ['cpp', 'c'],
  version: '1.0.0',
  meson_version: '>=0.53',
  default_options: ['cpp_std=c++20', 'warning_level=1'],
)

is_windows = target_machine.system() == 'windows'


cxx = meson.get_compiler('cpp')
gtk3 = dependency('gtk+-3.0', version: '>=3.22')
gtkmm = dependency('gtkmm-3.0', version: '>=3.0.0')
epoxy = dependency('epoxy')
eigen = dependency('eigen3')
glm = dependency('glm')
gst = dependency('gstreamer-1.0')
if is_windows
	hidapi = dependency('hidapi')
else
	hidapi = dependency('hidapi-hidraw')
endif

src = files(
  'src/usbkvm.cpp',
  'src/mshal.cpp',
  'src/i2c_util.cpp',
  'src/usbkvm_mcu.cpp',
  'src/usbkvm_device.cpp',
  'src/main_window.cpp',
  'src/type_window.cpp'
)

gnome = import('gnome')
resources = gnome.compile_resources(
  'resources',
  'src/usbkvm.gresource.xml',
  c_name: 'usbkvm_resources',
  source_dir: 'src'
)

cpp_args = []

stdlibs = []
is_libstdcpp = cxx.get_define('__GLIBCXX__', prefix: '#include <vector>') != ''
if is_libstdcpp
    message('using libstdc++')
else
    message('not using libstdc++')
endif
if is_libstdcpp
    stdlibs += cxx.find_library('stdc++')
    stdlibs += cxx.find_library('stdc++fs', required:false)
endif

build_dependencies = [gtk3, gtkmm, gst, hidapi]

prog_python = find_program('python3')

keymap_format = ['x11', 'usb']
keymap_input = ['keycodemapdb/tools/keymap-gen', 'keycodemapdb/data/keymaps.csv']
keymap_c = custom_target(
    'keymap.c',
    output : 'keymap.c',
    input : keymap_input,
    command : [prog_python, '@INPUT0@', 'code-map', '--lang=stdc', '@INPUT1@'] + keymap_format,
    capture : true
)

keymap_h = custom_target(
    'keymap.h',
    output : 'keymap.h',
    input : keymap_input,
    command : [prog_python, '@INPUT0@', 'code-map', '--lang=stdc-header', '@INPUT1@'] + keymap_format,
    capture : true
)

mslib = custom_target(
    'mslib',
    output: ['mslib.a', 'mslib.h'],
    input: 'ms-tools/lib/mslib.go',
    command: ['go', 'build', '-C',  join_paths(meson.current_source_dir(), 'ms-tools/lib'), '-o', join_paths(meson.current_build_dir(), '@OUTPUT0@'), '-buildmode=c-archive', 'mslib.go']
    
)

include_directories = [
  include_directories('src'),
]

usbkvm = executable('usbkvm',
    [src,resources,keymap_c,keymap_h,mslib],
    dependencies: [build_dependencies],
    #link_with: [solvespace, clipper],
    cpp_args: cpp_args,
    include_directories: include_directories,
    gui_app: true, 
    install: true
)

