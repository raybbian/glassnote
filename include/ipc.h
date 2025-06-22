#ifndef _GN_IPC_H
#define _GN_IPC_H

#include <wayland-util.h>

#include "glassnote.h"

int setup_dbus(struct gn_state *state);
void cleanup_dbus(struct gn_state *state);

#endif
