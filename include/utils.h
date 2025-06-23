#ifndef _GN_UTILS_H
#define _GN_UTILS_H

#include <math.h>

struct gn_vec2 {
    float x, y;
};

struct gn_vec3 {
    float x, y, z;
};

struct gn_box {
    struct gn_vec2 pos;
    struct gn_vec2 size;
};

static inline float gn_vec2_dot(struct gn_vec2 a, struct gn_vec2 b) {
    return a.x * b.x + a.y * b.y;
}

static inline float gn_vec2_norm_sq(struct gn_vec2 vec) {
    return vec.x * vec.x + vec.y * vec.y;
}

static inline float gn_vec2_norm(struct gn_vec2 vec) {
    return sqrtf(gn_vec2_norm_sq(vec));
}

static inline struct gn_vec2 gn_vec2_add(struct gn_vec2 a, struct gn_vec2 b) {
    return (struct gn_vec2){a.x + b.x, a.y + b.y};
}

static inline struct gn_vec2 gn_vec2_minus(struct gn_vec2 a, struct gn_vec2 b) {
    return (struct gn_vec2){a.x - b.x, a.y - b.y};
}

static inline float gn_vec2_perp_dist(struct gn_vec2 p, struct gn_vec2 a,
                                      struct gn_vec2 b) {
    float A = b.y - a.y;
    float B = a.x - b.x;
    float C = b.x * a.y - a.x * b.y;
    return fabsf(A * p.x + B * p.y + C) / sqrtf(A * A + B * B);
}

#endif
