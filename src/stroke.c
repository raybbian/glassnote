#include <stdio.h>
#include <stdlib.h>

#include "glassnote.h"
#include "stroke.h"
#include "utils.h"

#define STROKE_SIMPLIFICATION_THRESHOLD 1.5f

struct gn_stroke *create_stroke(struct gn_state *state, double width,
                                int32_t color) {
    if (state->n_strokes == state->c_strokes) {
        state->c_strokes *= 2;
        state->strokes = realloc(state->strokes,
                                 state->c_strokes * sizeof(struct gn_stroke));

        if (state->strokes == NULL) {
            fprintf(stderr, "Failed to allocate memory for more strokes\n");
            return NULL;
        }
    }

    struct gn_stroke *stroke = &state->strokes[state->n_strokes];
    state->n_strokes++;

    stroke->capacity = STROKE_DEFAULT_CAPACITY;
    stroke->n_pts = 0;
    stroke->pts_reported = 0;
    stroke->width = width;
    stroke->color = color;
    stroke->pts = calloc(stroke->capacity, sizeof(struct gn_point));

    if (stroke->pts == NULL) {
        fprintf(stderr, "Failed to allocate memory for stroke points\n");
        return NULL;
    }
    return stroke;
}

void extend_stroke(struct gn_stroke *stroke, double x, double y) {
    stroke->pts_reported++;

    if (stroke->seg_st + 1 < stroke->n_pts) {
        struct gn_point n_pt = {x, y};
        float max_dist = 0.0;
        size_t index = -1;

        for (size_t i = stroke->seg_st + 1; i < stroke->n_pts; i++) {
            float dist =
                gn_perp_dist(stroke->pts[i], stroke->pts[stroke->seg_st], n_pt);
            if (dist > max_dist) {
                max_dist = dist;
                index = i;
            }
        }

        if (max_dist < STROKE_SIMPLIFICATION_THRESHOLD) {
            goto add_point;
        }

        stroke->seg_st++;
        for (size_t i = index; i < stroke->n_pts; i++) {
            stroke->pts[i - index + stroke->seg_st] = stroke->pts[i];
        }
        stroke->n_pts = stroke->n_pts - index + stroke->seg_st;
        // continue to add the point
    }

add_point:
    if (stroke->n_pts == stroke->capacity) {
        if (stroke->capacity >= STROKE_MAX_PTS) {
            fprintf(stderr, "Stroke has too many points\n");
            return;
        }

        stroke->capacity *= 2;
        stroke->pts =
            realloc(stroke->pts, stroke->capacity * sizeof(struct gn_point));

        if (stroke->pts == NULL) {
            fprintf(stderr, "Failed to allocate memory for more strokes\n");
            return;
        }
    }

    stroke->pts[stroke->n_pts].x = x;
    stroke->pts[stroke->n_pts].y = y;
    stroke->n_pts++;
}

void destroy_stroke(struct gn_stroke *stroke) {
    if (stroke == NULL) {
        return;
    }
    if (stroke->pts != NULL) {
        free(stroke->pts);
    }
}
