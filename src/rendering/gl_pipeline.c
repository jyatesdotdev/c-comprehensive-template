/**
 * @file gl_pipeline.c
 * @brief OpenGL pipeline implementation (requires HAS_OPENGL + GL headers).
 */
#include "rendering/gl_pipeline.h"

#ifdef HAS_OPENGL
#include <stdio.h>
#include <stdlib.h>

/** @brief Compile a GLSL shader from source and return its handle (0 on failure). */
GLuint gl_compile_shader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint gl_link_program(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof(log), NULL, log);
        fprintf(stderr, "Program link error: %s\n", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

GLuint gl_shader_from_file(const char *path, GLenum type) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *src = malloc((size_t)len + 1);
    if (!src) {
        fclose(f);
        return 0;
    }
    fread(src, 1, (size_t)len, f);
    src[len] = '\0';
    fclose(f);
    GLuint s = gl_compile_shader(src, type);
    free(src);
    return s;
}

GLuint gl_program_from_source(const char *vert_src, const char *frag_src) {
    GLuint vs = gl_compile_shader(vert_src, GL_VERTEX_SHADER);
    if (!vs) return 0;
    GLuint fs = gl_compile_shader(frag_src, GL_FRAGMENT_SHADER);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }
    GLuint prog = gl_link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

#endif /* HAS_OPENGL */
