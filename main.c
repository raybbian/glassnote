#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

#include "glassnote.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static const struct wl_callback_listener output_frame_listener;

static void send_frame(struct gn_state *state) {
    struct gn_output *output = &state->output;
    if (!output->configured) {
        return;
    }

    glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
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
        output->configured = true;

        output->egl_window =
            wl_egl_window_create(output->surface, width, height);
        if (output->egl_window == EGL_NO_SURFACE) {
            fprintf(stderr, "Can't create egl window\n");
            exit(EXIT_FAILURE);
        }
        output->egl_surface = eglCreateWindowSurface(
            state->egl_display, state->egl_config,
            (EGLNativeWindowType)output->egl_window, NULL);

        eglMakeCurrent(state->egl_display, output->egl_surface,
                       output->egl_surface, state->egl_context);
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
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = NULL,
};

int main(int argc, char **argv) {
    struct gn_state state = {};

    state.display = wl_display_connect(NULL);
    if (!state.display) {
        fprintf(stderr, "Failed to connect to Wayland\n");
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

    state.egl_display = eglGetDisplay((EGLNativeDisplayType)state.display);
    if (state.egl_display == EGL_NO_DISPLAY) {
        fprintf(stderr, "Can't create egl display\n");
        return EXIT_FAILURE;
    }

    if (eglInitialize(state.egl_display, NULL, NULL) != EGL_TRUE) {
        fprintf(stderr, "Can't initialize egl display\n");
        return EXIT_FAILURE;
    }

    const EGLint config_attribs[] = {EGL_SURFACE_TYPE,
                                     EGL_WINDOW_BIT,
                                     EGL_RED_SIZE,
                                     8,
                                     EGL_GREEN_SIZE,
                                     8,
                                     EGL_BLUE_SIZE,
                                     8,
                                     EGL_ALPHA_SIZE,
                                     8,
                                     EGL_RENDERABLE_TYPE,
                                     EGL_OPENGL_BIT,
                                     EGL_NONE};
    EGLint num_configs;
    eglChooseConfig(state.egl_display, config_attribs, &state.egl_config, 1,
                    &num_configs);

    const EGLint ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    state.egl_context = eglCreateContext(state.egl_display, state.egl_config,
                                         EGL_NO_CONTEXT, ctx_attribs);

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
    zwlr_layer_surface_v1_add_listener(state.output.layer_surface,
                                       &layer_surface_listener, &state);

    wl_surface_commit(state.output.surface);

    state.running = true;
    while (state.running && wl_display_dispatch(state.display) != -1) {
        ;
    }

    // Ensure compositor has unmapped surfaces
    wl_display_roundtrip(state.display);

    if (state.output.frame_callback) {
        wl_callback_destroy(state.output.frame_callback);
    }
    eglDestroySurface(state.egl_display, state.output.egl_surface);
    wl_egl_window_destroy(state.output.egl_window);
    zwlr_layer_surface_v1_destroy(state.output.layer_surface);
    wl_surface_destroy(state.output.surface);

    eglDestroyContext(state.egl_display, state.egl_context);
    eglTerminate(state.egl_display);
    zwlr_layer_shell_v1_destroy(state.layer_shell);
    wl_compositor_destroy(state.compositor);
    wl_registry_destroy(state.registry);
    wl_display_disconnect(state.display);

    return EXIT_SUCCESS;
}
