#include <GLES3/gl32.h>
#define __USE_GNU
#include <math.h>
#undef __USE_GNU
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glassnote.h"
#include "stroke.h"
#include "utils.h"

#define GL_UTILS_SHDR_VERSION "#version 320 es\n"
#define GL_UTILS_SHDR_SOURCE(x) #x

#define GN_LINES_ROUND_RES 8
#define GN_LINES_INSTANCE_SZ (6 * GN_LINES_ROUND_RES + 6)

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

    GLint status;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        int32_t info_len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &info_len);
        GLchar *info = malloc(info_len);
        glGetShaderInfoLog(s, info_len, &info_len, info);
        fprintf(stderr, "Shader compile error: \n%s\n", info);
        free(info);
        exit(EXIT_FAILURE);
    }
    return s;
}

static GLuint link_program(GLuint vs, GLuint fs) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    GLint status;
    glGetProgramiv(p, GL_LINK_STATUS, &status);
    if (!status) {
        int32_t info_len = 0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &info_len);
        info_len += 4096;
        GLchar *info = malloc(info_len);
        glGetProgramInfoLog(p, info_len, &info_len, info);
        fprintf(stderr, "Program link error: \n%s\n", info);
        free(info);
        exit(EXIT_FAILURE);
    }
    return p;
}

void init_gl(struct gn_state *state) {
    // clang-format off
    static const char *vs_src = 
        GL_UTILS_SHDR_VERSION 
        GL_UTILS_SHDR_SOURCE(
            layout(location = 0) in vec3 a_pos; 
            layout(location = 1) in vec2 a_pt1;
            layout(location = 2) in vec2 a_pt2;
            uniform vec2 u_resolution;
            uniform float u_width;

            void main() {
                vec2 xBasis = normalize(a_pt2 - a_pt1);
                vec2 yBasis = vec2(-xBasis.y, xBasis.x);
                vec2 offsetA = a_pt1 + u_width * (a_pos.x * xBasis + a_pos.y * yBasis);
                vec2 offsetB = a_pt2 + u_width * (a_pos.x * xBasis + a_pos.y * yBasis);
                vec2 pt = mix(offsetA, offsetB, a_pos.z);
                vec2 clipSpace = pt / u_resolution * 2.0 - 1.0;
                gl_Position = vec4(clipSpace * vec2(1.0, -1.0), 0.0, 1.0);
            }
        );
    static const char *fs_src =
        GL_UTILS_SHDR_VERSION
        GL_UTILS_SHDR_SOURCE(
            precision mediump float;
            out vec4 fragColor;
            uniform vec4 u_color;

            void main() {
              fragColor = u_color;
            }
        );
    // clang-format on

    struct gn_lines_device *gl = &state->gl;

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    gl->program_id = link_program(vs, fs);
    glDetachShader(gl->program_id, vs);
    glDetachShader(gl->program_id, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    gl->uniforms.u_resolution =
        glGetUniformLocation(gl->program_id, "u_resolution");
    gl->uniforms.u_width = glGetUniformLocation(gl->program_id, "u_width");
    gl->uniforms.u_color = glGetUniformLocation(gl->program_id, "u_color");

    gl->attribs.a_pos = glGetAttribLocation(gl->program_id, "a_pos");
    gl->attribs.a_pt1 = glGetAttribLocation(gl->program_id, "a_pt1");
    gl->attribs.a_pt2 = glGetAttribLocation(gl->program_id, "a_pt2");

    struct gn_vec3 line_instance[GN_LINES_INSTANCE_SZ] = {
        {0, -0.5, 0}, {0, -0.5, 1}, {0, 0.5, 1},
        {0, -0.5, 0}, {0, 0.5, 1},  {0, 0.5, 0},
    };
    size_t ptr = 6;
    for (int cap = 0; cap < 2; cap++) {
        float base_angle = cap == 0 ? M_PI_2f : 3.f * M_PI_2f;
        float z = cap == 0 ? 0.f : 1.f;
        for (size_t i = 0; i < GN_LINES_ROUND_RES; i++) {
            float theta0 = base_angle + (i * M_PIf) / GN_LINES_ROUND_RES;
            float theta1 = base_angle + ((i + 1) * M_PIf) / GN_LINES_ROUND_RES;
            line_instance[ptr++] = (struct gn_vec3){0, 0, z};
            line_instance[ptr++] =
                (struct gn_vec3){0.5 * cosf(theta0), 0.5 * sinf(theta0), z};
            line_instance[ptr++] =
                (struct gn_vec3){0.5 * cosf(theta1), 0.5 * sinf(theta1), z};
        }
    }

    glUseProgram(gl->program_id);
    glGenVertexArrays(1, &gl->vao);
    glGenBuffers(1, &gl->mesh_vbo);
    glGenBuffers(1, &gl->instance_vbo);

    glBindVertexArray(gl->vao);

    glBindBuffer(GL_ARRAY_BUFFER, gl->mesh_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_instance), line_instance,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(gl->attribs.a_pos);
    glVertexAttribPointer(gl->attribs.a_pos, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct gn_vec3), 0);
    glVertexAttribDivisor(gl->attribs.a_pos, 0);

    glBindBuffer(GL_ARRAY_BUFFER, gl->instance_vbo);
    glBufferData(GL_ARRAY_BUFFER, STROKE_MAX_PTS * sizeof(struct gn_vec2), NULL,
                 GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(gl->attribs.a_pt1);
    glVertexAttribPointer(gl->attribs.a_pt1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct gn_vec2), 0);
    glVertexAttribDivisor(gl->attribs.a_pt1, 1);

    glEnableVertexAttribArray(gl->attribs.a_pt2);
    glVertexAttribPointer(
        gl->attribs.a_pt2, 2, GL_FLOAT, GL_FALSE, sizeof(struct gn_vec2),
        (void *)sizeof(struct gn_vec2)); // start one vec2 later
    glVertexAttribDivisor(gl->attribs.a_pt2, 1);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ZERO, GL_ONE, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
}

void cleanup_gl(struct gn_state *state) {
    struct gn_lines_device *gl = &state->gl;

    glUseProgram(0);
    glDeleteProgram(gl->program_id);
    glDeleteBuffers(1, &gl->mesh_vbo);
    glDeleteVertexArrays(1, &gl->vao);
}

void render(struct gn_state *state) {
    struct gn_lines_device *gl = &state->gl;

    float buf[4];
    unpack_rgba_i32_premul(state->bg_colors[state->active], buf);

    // glClearColor wants premultiplied alpha values
    glClearColor(buf[0], buf[1], buf[2], buf[3]);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(gl->program_id);
    glBindVertexArray(gl->vao);
    glUniform2f(gl->uniforms.u_resolution, (float)state->output.width,
                (float)state->output.height);

    glBindBuffer(GL_ARRAY_BUFFER, gl->instance_vbo);
    for (size_t i = 0; i < state->n_strokes; i++) {
        // printf("stroke %zu has n_pts %zu reported pts %zu\n", i,
        //        state->strokes[i].n_pts, state->strokes[i].pts_reported);
        struct gn_stroke *stroke = &state->strokes[i];

        unpack_rgba_i32(stroke->color, buf);
        glUniform4f(gl->uniforms.u_color, buf[0], buf[1], buf[2],
                    state->active ? 1.0 : 0.3);
        glUniform1f(gl->uniforms.u_width, stroke->width);

        glBufferSubData(GL_ARRAY_BUFFER, 0,
                        stroke->n_pts * sizeof(struct gn_vec2), stroke->pts);
        glDrawArraysInstanced(GL_TRIANGLES, 0, GN_LINES_INSTANCE_SZ,
                              stroke->n_pts - 1);
    }
}
