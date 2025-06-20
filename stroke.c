#include <stdio.h>
#include <stdlib.h>

#include "glassnote.h"
#include "stroke.h"
#include "utils.h"

struct gn_stroke *create_stroke(struct gn_state *state, double line_width,
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
    stroke->line_width = line_width;
    stroke->color = color;
    stroke->pts = calloc(stroke->capacity, sizeof(struct gn_point));

    if (stroke->pts == NULL) {
        fprintf(stderr, "Failed to allocate memory for stroke points\n");
        return NULL;
    }
    return stroke;
}

void extend_stroke(struct gn_stroke *stroke, double x, double y) {
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

    // TODO: simplify stroke?

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
    free(stroke);
}
