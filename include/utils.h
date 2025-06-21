#ifndef _GN_UTILS_H
#define _GN_UTILS_H

#include <math.h>

struct gn_point {
    float x;
    float y;
};

struct gn_box {
    struct gn_point pos;
    struct gn_point size;
};

static inline float gn_dot(struct gn_point a, struct gn_point b) {
    return a.x * b.x + a.y * b.y;
}

static inline float gn_norm_sq(struct gn_point vec) {
    return vec.x * vec.x + vec.y * vec.y;
}

static inline float gn_norm(struct gn_point vec) {
    return sqrtf(gn_norm_sq(vec));
}

static inline struct gn_point gn_add(struct gn_point a, struct gn_point b) {
    return (struct gn_point){a.x + b.x, a.y + b.y};
}

static inline struct gn_point gn_minus(struct gn_point a, struct gn_point b) {
    return (struct gn_point){a.x - b.x, a.y - b.y};
}

static inline float gn_perp_dist(struct gn_point p, struct gn_point a,
                                 struct gn_point b) {
    float A = b.y - a.y;
    float B = a.x - b.x;
    float C = b.x * a.y - a.x * b.y;
    return fabsf(A * p.x + B * p.y + C) / sqrtf(A * A + B * B);
}

#endif
