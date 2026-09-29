#include "Shader.h"

#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <string>
#include <vector>

namespace {

GLuint compile(GLenum type, const char* source, const char* label) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len) + 1, '\0');
        glGetShaderInfoLog(shader, len, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Simulador: error compilando el shader ") + label + ":\n" + log.data());
    }
    return shader;
}

}  // namespace

Shader::Shader(const char* vertexSource, const char* fragmentSource) {
    const GLuint vs = compile(GL_VERTEX_SHADER, vertexSource, "basic.vert");
    GLuint fs = 0;
    try {
        fs = compile(GL_FRAGMENT_SHADER, fragmentSource, "basic.frag");
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);  // ya enlazados: los objetos individuales sobran
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<std::size_t>(len) + 1, '\0');
        glGetProgramInfoLog(program_, len, nullptr, log.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error(std::string("Simulador: error enlazando el shader:\n") + log.data());
    }
}

Shader::~Shader() {
    if (program_ != 0) glDeleteProgram(program_);
}

void Shader::setMat4(const char* name, const glm::mat4& value) const {
    glUniformMatrix4fv(glGetUniformLocation(program_, name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(const char* name, const glm::vec3& value) const {
    glUniform3fv(glGetUniformLocation(program_, name), 1, glm::value_ptr(value));
}
