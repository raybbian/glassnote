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

libsystemd = dependency('libsystemd')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.32')
egl = dependency('egl')
open_gl = dependency('glesv2')
wayland_egl = dependency('wayland-egl')
math = cc.find_library('m')
xkbcommon = dependency('xkbcommon')

subdir('protocol')

executable(
    'glassnote',
    [
        'src/main.c',
        'src/render.c',
        'src/stroke.c',
        'src/seat.c',
        'src/ipc.c',
        protos_src,
    ],
    dependencies: [
        libsystemd,
        wayland_client,
        egl,
        open_gl, 
        wayland_egl,
        math,
        xkbcommon
    ],
    include_directories: [
        'include',
        'shared',
    ],
    install: true,
)

executable(
    'gnctl',
    [
        'gnctl/main.c'
    ],
    dependencies: [
        libsystemd,
    ],
    include_directories: [
        'shared',
    ],
    install: true,
)
