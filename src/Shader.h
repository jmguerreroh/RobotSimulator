// Shader.h - Programa GLSL (vertex + fragment) con RAII.
#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>

class Shader {
public:
    Shader(const char* vertexSource, const char* fragmentSource);  // lanza std::runtime_error
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const { glUseProgram(program_); }
    void setMat4(const char* name, const glm::mat4& value) const;
    void setVec3(const char* name, const glm::vec3& value) const;

private:
    GLuint program_ = 0;
};
