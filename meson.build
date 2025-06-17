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

cairo = dependency('cairo')
realtime = cc.find_library('rt')
wayland_client = dependency('wayland-client')
wayland_cursor = dependency('wayland-cursor')
wayland_protos = dependency('wayland-protocols', version: '>=1.32')
xkbcommon = dependency('xkbcommon')

subdir('protocol')

executable(
    'glassnote',
    [
        'main.c',
        protos_src,
    ],
    dependencies: [
        cairo,
        realtime,
        wayland_client,
        wayland_cursor,
        xkbcommon,
    ],
    include_directories: 'include',
    install: true,
)
