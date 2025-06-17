#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>

#include "glassnote.h"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *iface,
                                   uint32_t version) {
    struct gn_state *state = data;
    if (strcmp(iface, wl_compositor_interface.name) == 0) {
        state->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(iface, wl_shm_interface.name) == 0) {
        state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0) {
        state->layer_shell =
            wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 1);
    }
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_handle_global,
    .global_remove = NULL,
};

static void layer_surface_handle_configure(
    void *data, struct zwlr_layer_surface_v1 *surface, uint32_t serial,
    uint32_t width, uint32_t height) {
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    // TODO: update buffer for resize and send frame again
}

static void layer_surface_handle_closed(void *data,
                                        struct zwlr_layer_surface_v1 *surface) {
    // TODO: cleanup resources
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_handle_configure,
    .closed = layer_surface_handle_closed,
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
        fprintf(stderr, "Compositor doesn't support wl_compsitor\n");
        return EXIT_FAILURE;
    }
    if (state.shm == NULL) {
        fprintf(stderr, "Compositor doesn't support wl_shm\n");
        return EXIT_FAILURE;
    }
    if (state.layer_shell == NULL) {
        fprintf(stderr, "Compositor doesn't support zwlr_layer_shell_v1");
        return EXIT_FAILURE;
    }

    struct wl_surface *surface = wl_compositor_create_surface(state.compositor);

    struct zwlr_layer_surface_v1 *layer_surface =
        zwlr_layer_shell_v1_get_layer_surface(state.layer_shell, surface, NULL,
                                              ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
                                              "glassnote");
    zwlr_layer_surface_v1_set_size(layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_anchor(layer_surface,
                                     ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
    zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
    zwlr_layer_surface_v1_add_listener(layer_surface, &layer_surface_listener,
                                       state.display);

    wl_surface_commit(surface);
    wl_display_roundtrip(state.display);

    state.running = true;
    while (state.running && wl_display_dispatch(state.display) != -1) {
        //
    }

    wl_display_roundtrip(state.display);

    zwlr_layer_shell_v1_destroy(state.layer_shell);
    wl_compositor_destroy(state.compositor);
    wl_shm_destroy(state.shm);
    wl_registry_destroy(state.registry);
    wl_display_disconnect(state.display);

    return EXIT_SUCCESS;
}
