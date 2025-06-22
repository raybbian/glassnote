#include <GL/gl.h>
#include <GLES3/gl32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glassnote.h"
#include "stroke.h"
#include "utils.h"

static const float inv255 = 1.0f / 255.0f;

static void unpack_rgba_i32(int32_t rgba, float out[static 4]) {
    out[0] = ((rgba >> 24) & 0xFF) * inv255;
    out[1] = ((rgba >> 16) & 0xFF) * inv255;
    out[2] = ((rgba >> 8) & 0xFF) * inv255;
    out[3] = (rgba & 0xFF) * inv255;
}

static void unpack_rgba_i32_premul(int32_t rgba, float out[static 4]) {
    out[3] = (rgba & 0xFF) * inv255;
    out[0] = ((rgba >> 24) & 0xFF) * inv255 * out[3];
    out[1] = ((rgba >> 16) & 0xFF) * inv255 * out[3];
    out[2] = ((rgba >> 8) & 0xFF) * inv255 * out[3];
}

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetShaderInfoLog(s, sizeof(buf), NULL, buf);
        fprintf(stderr, "Shader compile error: %s\n", buf);
        exit(1);
    }
    return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char buf[512];
        glGetProgramInfoLog(p, sizeof(buf), NULL, buf);
        fprintf(stderr, "Program link error: %s\n", buf);
        exit(1);
    }
    return p;
}

void init_gl(struct gn_state *state) {
    static const char *vs_src =
        "#version 320 es\n"
        "layout(location = 0) in vec2 a_pos;\n"
        "uniform vec2 u_resolution;\n"
        "void main() {\n"
        "vec2 clipSpace = a_pos / u_resolution * 2.0 - 1.0;\n"
        "  gl_Position = vec4(clipSpace * vec2(1.0, -1.0), 0.0, 1.0);\n"
        "}\n";
    static const char *fs_src = "#version 320 es\n"
                                "precision mediump float;\n"
                                "out vec4 fragColor;\n"
                                "uniform vec4 u_color;\n"
                                "void main() {\n"
                                "  fragColor = u_color;\n"
                                "}\n";
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    state->line_prog = link_program(vs, fs);
    glDetachShader(state->line_prog, vs);
    glDetachShader(state->line_prog, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    state->line_res_loc =
        glGetUniformLocation(state->line_prog, "u_resolution");
    state->line_col_loc = glGetUniformLocation(state->line_prog, "u_color");

    glUseProgram(state->line_prog);
    glGenVertexArrays(1, &state->line_vao);
    glGenBuffers(1, &state->line_vbo);
    glBindVertexArray(state->line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->line_vbo);
    glBufferData(GL_ARRAY_BUFFER, STROKE_MAX_PTS * sizeof(struct gn_point),
                 NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void cleanup_gl(struct gn_state *state) {
    glUseProgram(0);
    glDeleteProgram(state->line_prog);
    glDeleteBuffers(1, &state->line_vbo);
    glDeleteVertexArrays(1, &state->line_vao);
}

void render(struct gn_state *state) {
    float buf[4];
    unpack_rgba_i32_premul(state->bg_colors[state->active], buf);

    // glClearColor wants premultiplied alpha values
    glClearColor(buf[0], buf[1], buf[2], buf[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(state->line_prog);
    glBindVertexArray(state->line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->line_vbo);

    glUniform2f(state->line_res_loc, (float)state->output.width,
                (float)state->output.height);

    for (size_t i = 0; i < state->n_strokes; i++) {
        // printf("stroke %zu has n_pts %zu reported pts %zu\n", i,
        //        state->strokes[i].n_pts, state->strokes[i].pts_reported);
        struct gn_stroke *stroke = &state->strokes[i];

        unpack_rgba_i32(stroke->color, buf);
        glUniform4f(state->line_col_loc, buf[0], buf[1], buf[2],
                    state->active ? 1.0 : 0.3);
        glLineWidth(stroke->width);

        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        stroke->n_pts * sizeof(struct gn_point), stroke->pts);
        glDrawArrays(GL_LINE_STRIP, 0, stroke->n_pts);
    }
}
