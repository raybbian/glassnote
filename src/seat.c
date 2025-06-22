#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "cursor-shape-v1-client-protocol.h"
#include "glassnote.h"
#include "seat.h"
#include "stroke.h"

static void seat_handle_pressed(struct gn_seat *seat) {
    if (seat->cur_stroke != NULL) {
        return;
    }
    struct gn_state *state = seat->state;
    seat->cur_stroke = create_stroke(state, state->cur_stroke_width,
                                     state->colors[state->color_ind]);
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

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
                                  uint32_t time, wl_fixed_t surface_x,
                                  wl_fixed_t surface_y) {
    struct gn_seat *seat = data;
    if (seat->cur_stroke == NULL) {
        return;
    }

    double x = wl_fixed_to_double(surface_x);
    double y = wl_fixed_to_double(surface_y);
    extend_stroke(seat->cur_stroke, x, y);

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
    .leave = noop,
    .motion = pointer_handle_motion,
    .button = pointer_handle_button,
    .axis = noop,
};

static void keyboard_handle_keymap(void *data, struct wl_keyboard *wl_keyboard,
                                   const uint32_t format, const int32_t fd,
                                   const uint32_t size) {
    struct gn_seat *seat = data;
    switch (format) {
    case WL_KEYBOARD_KEYMAP_FORMAT_NO_KEYMAP:
        seat->xkb_keymap = xkb_keymap_new_from_names(
            seat->state->xkb_context, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
        break;
    case WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1:;
        void *buffer;
        if ((buffer = mmap(NULL, size - 1, PROT_READ, MAP_PRIVATE, fd, 0)) ==
            MAP_FAILED) {
            fprintf(stderr, "mmap failed\n");
            exit(EXIT_FAILURE);
        }
        seat->xkb_keymap = xkb_keymap_new_from_buffer(
            seat->state->xkb_context, buffer, size - 1,
            XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
        munmap(buffer, size - 1);
        close(fd);
        break;
    }
    seat->xkb_state = xkb_state_new(seat->xkb_keymap);
}

static void keyboard_handle_key(void *data, struct wl_keyboard *wl_keyboard,
                                const uint32_t serial, const uint32_t time,
                                const uint32_t key, const uint32_t key_state) {
    struct gn_seat *seat = data;
    struct gn_state *state = seat->state;
    const xkb_keysym_t keysym =
        xkb_state_key_get_one_sym(seat->xkb_state, key + 8);

    switch (key_state) {
    case WL_KEYBOARD_KEY_STATE_PRESSED:
        switch (keysym) {
        case XKB_KEY_Escape:
            seat_handle_released(seat);
            state->running = false;
            break;
        case XKB_KEY_1:
        case XKB_KEY_2:
        case XKB_KEY_3:
        case XKB_KEY_4:
        case XKB_KEY_5:
            state->color_ind = (keysym & 0xF) - 1;
            break;
        case XKB_KEY_minus:
            state->cur_stroke_width =
                state->cur_stroke_width - 1.f < STROKE_MIN_WIDTH
                    ? STROKE_MIN_WIDTH
                    : state->cur_stroke_width - 1.f;
            break;
        case XKB_KEY_equal:
            state->cur_stroke_width =
                state->cur_stroke_width + 1.f > STROKE_MAX_WIDTH
                    ? STROKE_MAX_WIDTH
                    : state->cur_stroke_width + 1.f;
            break;
        }

    case WL_KEYBOARD_KEY_STATE_RELEASED:
        break;
    }
}

static void
keyboard_handle_modifiers(void *data, struct wl_keyboard *wl_keyboard,
                          const uint32_t serial, const uint32_t mods_depressed,
                          const uint32_t mods_latched,
                          const uint32_t mods_locked, const uint32_t group) {
    struct gn_seat *seat = data;
    xkb_state_update_mask(seat->xkb_state, mods_depressed, mods_latched,
                          mods_locked, 0, 0, group);
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = keyboard_handle_keymap,
    .enter = noop,
    .leave = noop,
    .key = keyboard_handle_key,
    .modifiers = keyboard_handle_modifiers,
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
        wl_keyboard_add_listener(seat->wl_keyboard, &keyboard_listener, seat);
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
        return;
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
    xkb_state_unref(seat->xkb_state);
    xkb_keymap_unref(seat->xkb_keymap);
    wl_seat_destroy(seat->wl_seat);
    free(seat);
}
