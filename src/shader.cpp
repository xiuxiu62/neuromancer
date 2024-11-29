#include "shader.hpp"

#include "core/logger.h"

void check_shader_compilation(GLuint shader) {
    GLint success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        error("Shader compilation failed: %s\n", infoLog);
    }
}
