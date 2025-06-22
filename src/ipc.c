#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <systemd/sd-bus-vtable.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <wayland-client-protocol.h>

#include "glassnote.h"
#include "gnctl.h"
#include "ipc.h"

static int on_show_overlay(sd_bus_message *m, void *userdata,
                           sd_bus_error *ret) {
    struct gn_state *state = userdata;
    bool success = false;

    if (state->output.configured && !state->active) {
        wl_surface_set_input_region(state->output.surface, NULL);
        set_output_dirty(state);
        state->active = true;
        wl_surface_commit(state->output.surface);
        success = true;
    }

    return sd_bus_reply_method_return(m, "b", success);
}

static int on_hide_overlay(sd_bus_message *m, void *userdata,
                           sd_bus_error *ret) {
    struct gn_state *state = userdata;
    bool success = false;

    if (state->output.configured && state->active) {
        wl_surface_set_input_region(state->output.surface, state->empty_region);
        set_output_dirty(state);
        state->active = false;
        wl_surface_commit(state->output.surface);
        success = true;
    }

    return sd_bus_reply_method_return(m, "b", success);
}

static const sd_bus_vtable ipc_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD(GN_SD_BUS_SHOW_CMD, "", "b", on_show_overlay,
                  SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD(GN_SD_BUS_HIDE_CMD, "", "b", on_hide_overlay,
                  SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END};

int setup_dbus(struct gn_state *state) {
    int r = sd_bus_open_user(&state->bus);
    if (r < 0) {
        return r;
    }
    r = sd_bus_request_name(state->bus, GN_SD_BUS_NAME, 0);
    if (r < 0) {
        sd_bus_unref(state->bus);
        return r;
    }
    r = sd_bus_add_object_vtable(state->bus, &state->bus_slot,
                                 GN_SD_BUS_OBJ_PATH, GN_SD_BUS_NAME, ipc_vtable,
                                 state);
    if (r < 0) {
        sd_bus_release_name(state->bus, GN_SD_BUS_NAME);
        sd_bus_unref(state->bus);
        return r;
    }
    return 0;
}

void cleanup_dbus(struct gn_state *state) {
    if (state->bus_slot) {
        sd_bus_slot_unref(state->bus_slot);
    }
    sd_bus_release_name(state->bus, GN_SD_BUS_NAME);
    if (state->bus) {
        sd_bus_unref(state->bus);
    }
}
