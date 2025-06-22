#ifndef _GN_SEAT_H
#define _GN_SEAT_H

#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "glassnote.h"

struct gn_seat {
    struct gn_state *state;
    struct wl_seat *wl_seat;
    struct wl_list link; // gn_state::seats

    struct wl_keyboard *wl_keyboard;
    struct xkb_keymap *xkb_keymap;
    struct xkb_state *xkb_state;

    struct wl_pointer *wl_pointer;
    struct wl_touch *wl_touch;

    struct gn_stroke *cur_stroke;
};

void create_seat(struct gn_state *state, struct wl_seat *wl_seat);
void destroy_seat(struct gn_seat *seat);

#endif
