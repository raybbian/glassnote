#ifndef _GLASSNOTE_H
#define _GLASSNOTE_H

#include <EGL/egl.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>
#include <wayland-client.h>
#include <wayland-egl.h>

#define STATE_INITIAL_STROKES 64

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

    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wp_cursor_shape_manager_v1 *cursor_shape_manager;

    EGLDisplay egl_display;
    EGLConfig egl_config;
    EGLContext egl_context;

    struct gn_output output;

    struct gn_stroke *strokes;
    size_t n_strokes, c_strokes;

    struct wl_list seats; // gn_seat::link

    GLuint line_prog, line_vao, line_vbo, line_res_loc;
};

void set_output_dirty(struct gn_state *state);

#endif
