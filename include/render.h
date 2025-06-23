#ifndef _GN_RENDER_H
#define _GN_RENDER_H

#include <GLES3/gl32.h>

struct gn_state;

struct gn_lines_device {
    GLuint program_id;
    GLuint vao;
    GLuint mesh_vbo;
    GLuint instance_vbo;

    struct gn_lines_uniforms {
        GLuint u_resolution;
        GLuint u_color;
        GLuint u_width;
    } uniforms;

    struct gn_lines_attributes {
        GLuint a_pos;
        GLuint a_pt1;
        GLuint a_pt2;
    } attribs;
};

int init_egl(struct gn_state *state);
void cleanup_egl(struct gn_state *state);
void init_gl(struct gn_state *state);
void cleanup_gl(struct gn_state *state);
void render(struct gn_state *state);

#endif
