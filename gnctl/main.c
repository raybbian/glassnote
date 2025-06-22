#include "gnctl.h"
#include <systemd/sd-bus.h>

int main(int argc, char **argv) {
    sd_bus *bus = NULL;
    int r;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        return 1;
    }

    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to bus: %s\n", strerror(-r));
        return 1;
    }

    if (strcmp(argv[1], "show") == 0) {
        r = sd_bus_call_method(bus, GN_SD_BUS_NAME, GN_SD_BUS_OBJ_PATH,
                               GN_SD_BUS_NAME, GN_SD_BUS_SHOW_CMD, NULL, NULL,
                               "");
    } else if (strcmp(argv[1], "hide") == 0) {
        r = sd_bus_call_method(bus, GN_SD_BUS_NAME, GN_SD_BUS_OBJ_PATH,
                               GN_SD_BUS_NAME, GN_SD_BUS_HIDE_CMD, NULL, NULL,
                               "");
    }

    if (r < 0) {
        fprintf(stderr, "DBus call failed: %s\n", strerror(-r));
        return 1;
    }

    sd_bus_unref(bus);
    return 0;
}
