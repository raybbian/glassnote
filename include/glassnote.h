#ifndef _GLASSNOTE_H
#define _GLASSNOTE_H

#include <stdbool.h>
#include <wayland-client.h>

struct gn_state {
    bool running;

    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_shm *shm;
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;
};

#endif
