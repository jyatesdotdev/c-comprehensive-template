/**
 * @file gl_pipeline.h
 * @brief OpenGL rendering pipeline helpers (requires HAS_OPENGL).
 *
 * Provides shader compilation, program linking, and basic pipeline setup.
 * Guarded behind HAS_OPENGL — when not available, serves as a reference
 * for the patterns used in real OpenGL code.
 */
#ifndef RENDERING_GL_PIPELINE_H
#define RENDERING_GL_PIPELINE_H

#include "core/error.h"
#include <stdint.h>

#ifdef HAS_OPENGL
#include <GL/gl.h>

/**
 * @brief Compile a shader from source.
 * @param source GLSL source code string.
 * @param type   Shader type (e.g. GL_VERTEX_SHADER, GL_FRAGMENT_SHADER).
 * @return Shader ID, or 0 on failure.
 */
GLuint gl_compile_shader(const char *source, GLenum type);

/**
 * @brief Link vertex + fragment shaders into a program.
 * @param vert Vertex shader ID.
 * @param frag Fragment shader ID.
 * @return Program ID, or 0 on failure.
 */
GLuint gl_link_program(GLuint vert, GLuint frag);

/**
 * @brief Load shader source from file, compile, and return shader ID.
 * @param path File path to GLSL source.
 * @param type Shader type (e.g. GL_VERTEX_SHADER).
 * @return Shader ID, or 0 on failure.
 */
GLuint gl_shader_from_file(const char *path, GLenum type);

/**
 * @brief Create a complete program from vertex + fragment source strings.
 * @param vert_src Vertex shader GLSL source.
 * @param frag_src Fragment shader GLSL source.
 * @return Program ID, or 0 on failure.
 */
GLuint gl_program_from_source(const char *vert_src, const char *frag_src);

#else /* !HAS_OPENGL — reference patterns only */

/*
 * OpenGL Pipeline Setup Pattern (reference):
 *
 * 1. Initialize context (via GLFW/SDL):
 *      glfwInit(); glfwCreateWindow(...);
 *      gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
 *
 * 2. Compile shaders:
 *      GLuint vs = gl_compile_shader(vert_src, GL_VERTEX_SHADER);
 *      GLuint fs = gl_compile_shader(frag_src, GL_FRAGMENT_SHADER);
 *
 * 3. Link program:
 *      GLuint prog = gl_link_program(vs, fs);
 *      glDeleteShader(vs); glDeleteShader(fs);
 *
 * 4. Setup VAO/VBO:
 *      glGenVertexArrays(1, &vao);
 *      glGenBuffers(1, &vbo);
 *      glBindVertexArray(vao);
 *      glBindBuffer(GL_ARRAY_BUFFER, vbo);
 *      glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
 *      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, offset);
 *      glEnableVertexAttribArray(0);
 *
 * 5. Render loop:
 *      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 *      glUseProgram(prog);
 *      glBindVertexArray(vao);
 *      glDrawArrays(GL_TRIANGLES, 0, vertex_count);
 *      glfwSwapBuffers(window);
 */

#endif /* HAS_OPENGL */
#endif /* RENDERING_GL_PIPELINE_H */
