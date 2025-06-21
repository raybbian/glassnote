project(
    'glassnote',
    'c',
    version: '0.0.1',
    license: 'MIT',
    meson_version: '>=0.59.0',
    default_options: ['c_std=c11', 'warning_level=2', 'werror=true'],
)

add_project_arguments('-Wno-unused-parameter', language: 'c')

cc = meson.get_compiler('c')

# cairo = dependency('cairo')
# realtime = cc.find_library('rt')
# wayland_cursor = dependency('wayland-cursor')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.32')
egl = dependency('egl')
open_gl = dependency('glesv2')
wayland_egl = dependency('wayland-egl')
math = cc.find_library('m')


# xkbcommon = dependency('xkbcommon')

subdir('protocol')

executable(
    'glassnote',
    [
        'main.c',
        'render.c',
        'stroke.c',
        'seat.c',
        protos_src,
    ],
    dependencies: [
        wayland_client,
        egl,
        open_gl, 
        wayland_egl,
        math
    ],
    include_directories: 'include',
    install: true,
)
