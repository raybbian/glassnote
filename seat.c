#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#include "cursor-shape-v1-client-protocol.h"
#include "glassnote.h"
#include "seat.h"
#include "stroke.h"

static void seat_handle_pressed(struct gn_seat *seat) {
    if (seat->cur_stroke != NULL) {
        return;
    }
    seat->cur_stroke = create_stroke(seat->state, 3.0f, 0xff0000ff);
}

static void seat_handle_released(struct gn_seat *seat) {
    seat->cur_stroke = NULL;
}

static void pointer_handle_enter(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *surface,
                                 wl_fixed_t surface_x, wl_fixed_t surface_y) {
    struct gn_seat *seat = data;

    // TODO: support compositors without shape manager
    struct wp_cursor_shape_device_v1 *device =
        wp_cursor_shape_manager_v1_get_pointer(
            seat->state->cursor_shape_manager, wl_pointer);
    wp_cursor_shape_device_v1_set_shape(
        device, serial, WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR);
    wp_cursor_shape_device_v1_destroy(device);
}

static void pointer_handle_leave(void *data, struct wl_pointer *wl_pointer,
                                 uint32_t serial, struct wl_surface *surface) {
    ;
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t time, wl_fixed_t surface_x,
                                  wl_fixed_t surface_y) {
    struct gn_seat *seat = data;
    if (seat->cur_stroke == NULL) {
        return;
    }

    double x = wl_fixed_to_double(surface_x);
    double y = wl_fixed_to_double(surface_y);
    extend_stroke(seat->cur_stroke, x / seat->state->output.width * 2 - 1,
                  -(y / seat->state->output.height * 2 - 1));

    set_output_dirty(seat->state);
}

static void pointer_handle_button(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t serial, uint32_t time,
                                  uint32_t button, uint32_t button_state) {
    struct gn_seat *seat = data;

    switch (button_state) {
    case WL_POINTER_BUTTON_STATE_PRESSED:
        seat_handle_pressed(seat);
        break;
    case WL_POINTER_BUTTON_STATE_RELEASED:
        seat_handle_released(seat);
        break;
    }
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_handle_enter,
    .leave = pointer_handle_leave,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = NULL,
};

static void seat_handle_capabilities(void *data, struct wl_seat *wl_seat,
                                     uint32_t capabilities) {
    struct gn_seat *seat = data;

    if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
        seat->wl_pointer = wl_seat_get_pointer(wl_seat);
        wl_pointer_add_listener(seat->wl_pointer, &pointer_listener, seat);
    }
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        seat->wl_keyboard = wl_seat_get_keyboard(wl_seat);
        // TODO: listener
    }
    if (capabilities & WL_SEAT_CAPABILITY_TOUCH) {
        seat->wl_touch = wl_seat_get_touch(wl_seat);
        // TODO: listener
    }
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_handle_capabilities,
};

void create_seat(struct gn_state *state, struct wl_seat *wl_seat) {
    struct gn_seat *seat = calloc(1, sizeof(struct gn_seat));
    if (seat == NULL) {
        fprintf(stderr, "Failed to allocate memory for seat\n");
    }

    seat->state = state;
    seat->wl_seat = wl_seat;
    seat->wl_pointer = NULL;
    seat->wl_keyboard = NULL;
    seat->wl_touch = NULL;

    wl_list_insert(&state->seats, &seat->link);
    wl_seat_add_listener(wl_seat, &seat_listener, seat);
}

void destroy_seat(struct gn_seat *seat) {
    wl_list_remove(&seat->link);
    if (seat->wl_pointer) {
        wl_pointer_destroy(seat->wl_pointer);
    }
    if (seat->wl_keyboard) {
        wl_keyboard_destroy(seat->wl_keyboard);
    }
    if (seat->wl_touch) {
        wl_touch_destroy(seat->wl_touch);
    }
    wl_seat_destroy(seat->wl_seat);
    free(seat);
}
