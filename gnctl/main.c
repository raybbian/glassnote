#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <systemd/sd-bus.h>

#include "gnctl.h"

void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s show\n"
            "  %s hide\n",
            prog, prog);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    sd_bus *bus = NULL;
    int r;
    r = sd_bus_open_user(&bus);
    if (r < 0) {
        fprintf(stderr, "Failed to connect to bus: %s\n", strerror(-r));
        return EXIT_FAILURE;
    }

    if (argc < 2) {
        sd_bus_unref(bus);
        fprintf(stderr, "Subcommand cannot be empty\n");
        usage(argv[0]);
    }

    sd_bus_message *reply = NULL;
    if (strcmp(argv[1], "show") == 0) {
        r = sd_bus_call_method(bus, GN_SD_BUS_NAME, GN_SD_BUS_OBJ_PATH,
                               GN_SD_BUS_NAME, GN_SD_BUS_SHOW_CMD, NULL, &reply,
                               "");
    } else if (strcmp(argv[1], "hide") == 0) {
        r = sd_bus_call_method(bus, GN_SD_BUS_NAME, GN_SD_BUS_OBJ_PATH,
                               GN_SD_BUS_NAME, GN_SD_BUS_HIDE_CMD, NULL, &reply,
                               "");
    } else {
        sd_bus_unref(bus);
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        usage(argv[0]);
    }

    if (r < 0) {
        if (-r == EHOSTUNREACH) {
            fprintf(stderr, "Glassnote is not running\n");
        } else {
            fprintf(stderr, "Unknown error: %s\n", strerror(-r));
        }
        sd_bus_unref(bus);
        return EXIT_FAILURE;
    }

    bool success;
    r = sd_bus_message_read(reply, "b", &success);
    sd_bus_message_unref(reply);
    if (r < 0) {
        fprintf(stderr, "Failed to parse reply: %s\n", strerror(-r));
        sd_bus_unref(bus);
        return EXIT_FAILURE;
    }

    sd_bus_unref(bus);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
