project('usbkvm', ['cpp', 'c'],
  version: '0.2.1',
  meson_version: '>=0.58',
  default_options: ['cpp_std=c++20', 'warning_level=1'],
)

is_windows = target_machine.system() == 'windows'


cxx = meson.get_compiler('cpp')
gtk3 = dependency('gtk+-3.0', version: '>=3.22')
gtkmm = dependency('gtkmm-3.0', version: '>=3.0.0')
glibmm = dependency('glibmm-2.4')
gst = dependency('gstreamer-1.0')
if is_windows
	hidapi = dependency('hidapi')
else
	hidapi = dependency('hidapi-hidraw')
endif

libpython = dependency('python3', required: false, disabler:true)
pybind11 = dependency('pybind11', required: false, disabler:true)
x11 = dependency('x11', required: false)

src_common = files(
  'src/mshal.cpp',
  'src/i2c_util.cpp',
  'src/usbkvm_mcu.cpp',
  'src/usbkvm_device.cpp',
)

src = files(
  'src/usbkvm.cpp',
  'src/usbkvm_application.cpp',
  'src/main_window.cpp',
  'src/type_window.cpp',
  'src/devices_window.cpp',
  'src/version.cpp',
  'src/about_dialog.cpp',
  'src/get_keymap.cpp',
  'src/util.cpp'
) + src_common

src_python = files(
  'src/usbkvm_py.cpp'
)

fs = import('fs')
use_bundled_fw = fs.is_dir('fw')
if use_bundled_fw
    message('using bundled firmware')
else
    message('using firmware from repo')
endif

resource_src_dirs = ['src']
if use_bundled_fw
    resource_src_dirs += 'fw'
else
    resource_src_dirs += '../fw/usbkvm/build/'
endif

gnome = import('gnome')
resources = gnome.compile_resources(
  'resources',
  'src/usbkvm.gresource.xml',
  c_name: 'usbkvm_resources',
  source_dir: resource_src_dirs
)

fs = import('fs')
has_git_dir = fs.is_dir('../.git')
version_deps = []
if has_git_dir
    message('including git commit')
    version_deps += ['../.git/HEAD', '../.git/index']
endif

prog_python = find_program('python3')

version_gen_cpp = custom_target(
    'version_gen.cpp',
    output : 'version_gen.cpp',
    input : ['make_version.py'],
    depend_files: version_deps,
    command : [prog_python, '@INPUT@', '@OUTPUT@', '@SOURCE_ROOT@', meson.project_version()],
)
src += version_gen_cpp

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

build_dependencies = [gtk3, gtkmm, gst, hidapi, x11]
build_dependencies_py = [hidapi, libpython, pybind11, glibmm]

keymap_output_format = 'usb'
keymap_input_formats = [
  'xorgevdev',
  'xorgkbd',
  'xorgxquartz',
  'xorgxwin',
  'osx',
  'x11',
  'atset1'
]

keymap_input = ['keycodemapdb/tools/keymap-gen', 'keycodemapdb/data/keymaps.csv']
keymap_sources = []

foreach input_format: keymap_input_formats
  
  target = 'keymap_@0@_@1@'.format(input_format, keymap_output_format)
  
  keymap_sources += custom_target(
      target+'.c',
      output : target+'.c',
      input : keymap_input,
      command : [prog_python, '@INPUT0@', 'code-map', '--lang=stdc', '@INPUT1@', input_format, keymap_output_format],
      capture : true
  )

  keymap_sources += custom_target(
      target + '.h',
      output : target + '.h',
      input : keymap_input,
      command : [prog_python, '@INPUT0@', 'code-map', '--lang=stdc-header', '@INPUT1@', input_format, keymap_output_format],
      capture : true
  )

endforeach

mslib = custom_target(
    'mslib',
    output: ['mslib.a', 'mslib.h'],
    input: 'ms-tools/lib/mslib.go',
    command: ['go', 'build', '-C',  join_paths(meson.current_source_dir(), 'ms-tools/lib'), '-o', join_paths(meson.current_build_dir(), '@OUTPUT0@'), '-buildmode=c-archive', 'mslib.go']
    
)

include_directories = [
  include_directories('src'),
]

if use_bundled_fw
    include_directories += 'fw'
else
    include_directories += ['../fw/usbkvm/Core/Inc/', '../fw/boot/Core/Inc/', '../fw/common/Inc/']
endif


datadir = get_option('datadir')
install_data('net.carrotindustries.usbkvm.desktop', install_dir:datadir / 'applications')
install_data('net.carrotindustries.usbkvm.metainfo.xml', install_dir:datadir / 'metainfo')
icondir = datadir / 'icons/hicolor'
install_data('src/icons/scalable/apps/usbkvm.svg', install_dir: icondir /  'scalable/apps', rename: 'net.carrotindustries.usbkvm.svg')

udevrulesdir_opt = get_option('udevrulesdir')
if udevrulesdir_opt == ''
  udev = dependency('udev', required: false)
  if udev.found()
    udevrulesdir = udev.get_variable('udevdir') / 'rules.d'
  endif
else
  udevrulesdir = udevrulesdir_opt
endif

if is_variable('udevrulesdir')
  message('installing udev rules to', udevrulesdir)
  install_data('70-usbkvm.rules', install_dir: udevrulesdir)
endif

if is_windows
	windows = import('windows')

	usbkvm_rc = custom_target(
        'usbkvm.rc',
        output : 'usbkvm.rc',
        input : ['make_rc.py'],
        command : [prog_python,  '@INPUT@', '@OUTPUT@', meson.project_version()],
    )

	rc_compiled =  windows.compile_resources(usbkvm_rc, include_directories:include_directories('.'))
else
	rc_compiled = []
endif

usbkvm = executable('usbkvm',
    [src,resources,keymap_sources,mslib, rc_compiled],
    dependencies: [build_dependencies],
    cpp_args: cpp_args,
    include_directories: include_directories,
    win_subsystem: 'windows',
    install: true
)

usbkvm_py = shared_module('usbkvm_py',
    [src_python, src_common, mslib],
    dependencies: build_dependencies_py,
    include_directories: include_directories,
    build_by_default: false,
    name_prefix: ''
)
