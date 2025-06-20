#ifndef _GN_STROKE_H
#define _GN_STROKE_H

#include <stddef.h>
#include <stdint.h>

#include "glassnote.h"

#define STROKE_DEFAULT_CAPACITY 64
#define STROKE_MAX_PTS 4096

struct gn_stroke {
    struct gn_point *pts;
    size_t n_pts;
    size_t capacity;

    float line_width;
    int32_t color;
};

struct gn_stroke *create_stroke(struct gn_state *state, double line_width,
                                int32_t color);
void extend_stroke(struct gn_stroke *stroke, double x, double y);
void destroy_stroke(struct gn_stroke *stroke);

#endif
