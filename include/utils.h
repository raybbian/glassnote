#ifndef _GN_UTILS_H
#define _GN_UTILS_H

struct gn_point {
    double x;
    double y;
};

struct gn_box {
    struct gn_point pos;
    struct gn_point size;
};

#endif
