#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GLES3/gl32.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "cursor-shape-v1-client-protocol.h"
#include "glassnote.h"
#include "ipc.h"
#include "render.h"
#include "seat.h"
#include "stroke.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

void noop() { ; }

static const struct wl_callback_listener output_frame_listener;

void set_output_dirty(struct gn_state *state) {
    struct gn_output *output = &state->output;
    output->dirty = true;
    if (output->frame_callback) {
        return;
    }

    output->frame_callback = wl_surface_frame(output->surface);
    wl_callback_add_listener(output->frame_callback, &output_frame_listener,
                             state);
    wl_surface_commit(output->surface);
}

static void send_frame(struct gn_state *state) {
    struct gn_output *output = &state->output;
    if (!output->configured) {
        return;
    }

    render(state);
    eglSwapBuffers(state->egl_display, output->egl_surface);

    output->frame_callback = wl_surface_frame(output->surface);
    wl_callback_add_listener(output->frame_callback, &output_frame_listener,
                             state);

    wl_surface_set_opaque_region(output->surface, NULL);
    wl_surface_commit(output->surface);
    output->dirty = false;
}

static void output_frame_handle_done(void *data, struct wl_callback *callback,
                                     uint32_t time) {
    struct gn_state *state = data;
    struct gn_output *output = &state->output;

    wl_callback_destroy(callback);
    output->frame_callback = NULL;

    if (output->dirty) {
        send_frame(state);
    }
}

static const struct wl_callback_listener output_frame_listener = {
    .done = output_frame_handle_done,
};

static void layer_surface_handle_configure(
    void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial,
    uint32_t width, uint32_t height) {
    struct gn_state *state = data;
    struct gn_output *output = &state->output;

    output->width = width;
    output->height = height;
    if (!output->configured) {
        // TODO: error checking
        output->configured = true;

        output->egl_window =
            wl_egl_window_create(output->surface, width, height);
        output->egl_surface = eglCreateWindowSurface(
            state->egl_display, state->egl_config,
            (EGLNativeWindowType)output->egl_window, NULL);

        eglMakeCurrent(state->egl_display, output->egl_surface,
                       output->egl_surface, state->egl_context);

        init_gl(state);
    }
    wl_egl_window_resize(output->egl_window, width, height, 0, 0);
    glViewport(0, 0, width, height);

    zwlr_layer_surface_v1_ack_configure(surface, serial);
    send_frame(state);
}

static void layer_surface_handle_closed(void *data,
                                        struct zwlr_layer_surface_v1 *surface) {
    struct gn_state *state = data;
    state->running = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed = layer_surface_handle_closed,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *iface,
                                   uint32_t version) {
    struct gn_state *state = data;
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        state->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell =
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    } else if (strcmp(iface, wl_seat_interface.name) == 0) {
        struct wl_seat *wl_seat =
            wl_registry_bind(registry, name, &wl_seat_interface, 1);
        create_seat(state, wl_seat);
    } else if (strcmp(iface, wp_cursor_shape_manager_v1_interface.name) == 0) {
        state->cursor_shape_manager = wl_registry_bind(
            registry, name, &wp_cursor_shape_manager_v1_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = NULL,
};

int main(int argc, char **argv) {
    struct gn_state state = {
        .active = true,
        .color_ind = 0,
        .bg_colors = {GN_STATE_INIT_BG_COLOR_INACTIVE,
                      GN_STATE_INIT_BG_COLOR_ACTIVE},
        .colors = {GN_STATE_INIT_COLOR_1, GN_STATE_INIT_COLOR_2,
                   GN_STATE_INIT_COLOR_3, GN_STATE_INIT_COLOR_4,
                   GN_STATE_INIT_COLOR_5},
        .cur_stroke_width = GN_STATE_INIT_WIDTH,
        .c_strokes = GN_STATE_INIT_STROKES,
    };

    state.strokes = calloc(state.c_strokes, sizeof(struct gn_stroke));
    if (state.strokes == NULL) {
        fprintf(stderr, "Failed to allocate space for strokes");
        return EXIT_FAILURE;
    }

    wl_list_init(&state.seats);

    if (setup_dbus(&state) != 0) {
        fprintf(stderr, "Failed to setup dbus IPC\n");
        return EXIT_FAILURE;
    }

    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland\n");
        return EXIT_FAILURE;
    }

    if ((state.xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS)) == NULL) {
        fprintf(stderr, "xkb_context_new failed\n");
        return EXIT_FAILURE;
    }

    state.registry = wl_display_get_registry(state.display);
    wl_registry_add_listener(state.registry, &registry_listener, &state);
    wl_display_roundtrip(state.display);

    if (state.compositor == NULL) {
        fprintf(stderr, "Compositor doesn't support wl_compositor\n");
        return EXIT_FAILURE;
    }
    if (state.layer_shell == NULL) {
        fprintf(stderr, "Compositor doesn't support zwlr_layer_shell_v1\n");
        return EXIT_FAILURE;
    }
    if (state.cursor_shape_manager == NULL) {
        fprintf(stderr, "Compositor doesn't support zwp_shape_manager\n");
        return EXIT_FAILURE;
    }

    state.empty_region = wl_compositor_create_region(state.compositor);

    if (init_egl(&state) == -1) {
        fprintf(stderr, "Could not initialize EGL\n");
        return EXIT_FAILURE;
    };

    state.output.surface = wl_compositor_create_surface(state.compositor);
    state.output.layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        state.layer_shell, state.output.surface, NULL,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "glassnote");
    zwlr_layer_surface_v1_set_size(state.output.layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_anchor(state.output.layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(state.output.layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        state.output.layer_surface,
        ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_ON_DEMAND);
    zwlr_layer_surface_v1_add_listener(state.output.layer_surface,
                                       &layer_surface_listener, &state);

    wl_surface_commit(state.output.surface);

    state.running = true;

    int wl_fd = wl_display_get_fd(state.display);
    int bus_fd = sd_bus_get_fd(state.bus);

    struct pollfd fds[2];
    fds[0].fd = wl_fd;
    fds[0].events = POLLIN;
    fds[1].fd = bus_fd;
    fds[1].events = POLLIN;

    while (state.running) {
        wl_display_flush(state.display);
        sd_bus_flush(state.bus);

        uint64_t usec;
        int ret = sd_bus_get_timeout(state.bus, &usec);
        if (ret < 0) {
            fprintf(stderr, "Could not get sd_bus_timeout");
            break;
        }
        int timeout_ms = (ret == 0) ? -1 : (int)(usec / 1000);

        int poll_ret = poll(fds, 2, timeout_ms);
        if (poll_ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "Poll error\n");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (wl_display_dispatch(state.display) < 0) {
                fprintf(stderr, "Wayland dispatch error\n");
                break;
            }
        }

        if (poll_ret == 0 || (fds[1].revents & POLLIN)) {
            int r;
            do {
                r = sd_bus_process(state.bus, NULL);
                if (r < 0) {
                    fprintf(stderr, "sd_bus_process: %s\n", strerror(-r));
                    exit(1);
                }
            } while (r > 0);
        }
    }

    struct gn_seat *seat_tmp, *seat;
    wl_list_for_each_safe(seat, seat_tmp, &state.seats, link) {
        destroy_seat(seat);
    }

    // Ensure compositor has unmapped surfaces
    wl_display_roundtrip(state.display);

    if (state.output.frame_callback) {
        wl_callback_destroy(state.output.frame_callback);
    }
    if (state.output.configured) {
        cleanup_gl(&state);
        eglDestroySurface(state.egl_display, state.output.egl_surface);
        wl_egl_window_destroy(state.output.egl_window);
        state.output.configured = false;
    }
    zwlr_layer_surface_v1_destroy(state.output.layer_surface);
    wl_surface_destroy(state.output.surface);

    cleanup_egl(&state);

    wl_region_destroy(state.empty_region);

    zwlr_layer_shell_v1_destroy(state.layer_shell);
    wp_cursor_shape_manager_v1_destroy(state.cursor_shape_manager);
    wl_compositor_destroy(state.compositor);
    wl_registry_destroy(state.registry);
    xkb_context_unref(state.xkb_context);
    wl_display_disconnect(state.display);

    cleanup_dbus(&state);

    for (size_t i = 0; i < state.n_strokes; i++) {
        destroy_stroke(&state.strokes[i]);
    }
    free(state.strokes);

    return EXIT_SUCCESS;
}
