#include <GLES3/gl32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glassnote.h"
#include "stroke.h"
#include "utils.h"

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
    static const char *vs_src = "#version 320 es\n"
                                "layout(location = 0) in vec2 a_pos;\n"
                                "void main() {\n"
                                "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
                                "}\n";
    static const char *fs_src = "#version 320 es\n"
                                "precision mediump float;\n"
                                "out vec4 fragColor;\n"
                                "void main() {\n"
                                "  fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
                                "}\n";
    state->line_prog = link_program(compile_shader(GL_VERTEX_SHADER, vs_src),
                                    compile_shader(GL_FRAGMENT_SHADER, fs_src));

    glUseProgram(state->line_prog);
    glGenVertexArrays(1, &state->line_vao);
    glGenBuffers(1, &state->line_vbo);
    glBindVertexArray(state->line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->line_vbo);
    glBufferData(GL_ARRAY_BUFFER, STROKE_MAX_PTS * sizeof(struct gn_point),
                 NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glLineWidth(3.0f);
}

void render(struct gn_state *state) {
    glClearColor(0.05f, 0.05f, 0.05f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(state->line_prog);
    glBindVertexArray(state->line_vao);
    glBindBuffer(GL_ARRAY_BUFFER, state->line_vbo);

    for (size_t i = 0; i < state->n_strokes; i++) {
        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        state->strokes[i].n_pts * sizeof(struct gn_point),
                        state->strokes[i].pts);
        glDrawArrays(GL_LINE_STRIP, 0, state->strokes[i].n_pts);
    }
}
