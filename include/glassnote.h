#ifndef _GLASSNOTE_H
#define _GLASSNOTE_H

#include <EGL/egl.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>
#include <systemd/sd-bus.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-server-core.h>

#include "render.h"

#define GN_STATE_INIT_STROKES 64
#define GN_STATE_INIT_WIDTH 3.f
#define GN_STATE_INIT_COLOR_1 0xd20f39ff
#define GN_STATE_INIT_COLOR_2 0xfe640bff
#define GN_STATE_INIT_COLOR_3 0xdf8e1dff
#define GN_STATE_INIT_COLOR_4 0x40a02bff
#define GN_STATE_INIT_COLOR_5 0x179299ff
#define GN_STATE_INIT_BG_COLOR_INACTIVE 0x00000000
#define GN_STATE_INIT_BG_COLOR_ACTIVE 0x00000044

struct gn_output {
    struct gn_state *state;

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
    bool active;

    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wp_cursor_shape_manager_v1 *cursor_shape_manager;
    struct xkb_context *xkb_context;

    struct sd_bus *bus;
    struct sd_bus_slot *bus_slot;

    struct wl_region *empty_region;

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;

    struct gn_output output;

    int32_t bg_colors[2];
    int32_t colors[5];
    size_t color_ind;
    float cur_stroke_width;
    struct gn_stroke *strokes;
    size_t n_strokes, c_strokes;

    struct wl_list seats; // gn_seat::link

    struct gn_lines_device gl;
};

void noop();
void set_output_dirty(struct gn_state *state);

#endif
