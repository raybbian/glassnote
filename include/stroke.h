#ifndef _GN_STROKE_H
#define _GN_STROKE_H

#include <stddef.h>
#include <stdint.h>
#include <wayland-util.h>

#include "glassnote.h"

#define STROKE_DEFAULT_CAPACITY 64
#define STROKE_MAX_PTS 4096

#define STROKE_MIN_WIDTH 1.f
#define STROKE_MAX_WIDTH 24.f

struct gn_stroke {
    struct gn_vec2 *pts;
    size_t n_pts;
    size_t capacity;

    float width;
    int32_t color;

    // index of segment start
    // https://www.inkandswitch.com/ink/notes/super-simple-stroke-simplification/
    size_t seg_st;
    // metrics for perf
    size_t pts_reported;
};

struct gn_stroke *create_stroke(struct gn_state *state, double width,
                                int32_t color);
void extend_stroke(struct gn_stroke *stroke, double x, double y);
void finish_stroke(struct gn_stroke *stroke);
void destroy_stroke(struct gn_stroke *stroke);

#endif
