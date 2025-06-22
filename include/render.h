#ifndef _GN_RENDER_H
#define _GN_RENDER_H

#include "glassnote.h"

void init_gl(struct gn_state *state);
void cleanup_gl(struct gn_state *state);
void render(struct gn_state *state);

#endif
