#ifndef _GLASSNOTE_H
#define _GLASSNOTE_H

#include <EGL/egl.h>
#include <stdbool.h>
#include <wayland-client.h>
#include <wayland-egl.h>

struct gn_box {
    int32_t x, y;
    int32_t width, height;
};

struct gn_output {
    struct gn_box geometry;
    struct gn_box logical_geometry;
    int32_t scale;

    struct wl_callback *frame_callback;
    bool configured;
    bool dirty;
    int32_t width, height;

    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer_surface;
    struct wl_egl_window *egl_window;
    EGLSurface egl_surface;
};

struct gn_state {
    bool running;

    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;

    struct gn_output output;
};

#endif
