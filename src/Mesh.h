// Mesh.h - Geometría base (VAO + VBO de vértices + VBO de instancias).
//
// Una Mesh guarda UNA forma (cubo, pirámide) y se dibuja muchas veces de una
// sola llamada (instanced rendering). Cada instancia aporta su matriz Model y
// su color, así que un tablero de 50x50 son unas pocas llamadas de dibujo, no
// miles. Los datos de instancias sólo se vuelven a subir cuando cambian.
#pragma once

#include <glad/glad.h>

#include <vector>

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct InstanceData {
    glm::mat4 model;  // Model: del espacio del objeto al del mundo
    glm::vec4 color;
};

class Mesh {
public:
    static Mesh cube();     // cubo unidad centrado en el origen (lado 1)
    static Mesh pyramid();  // pirámide de base cuadrada, punta hacia +X, en [-0.5, 0.5]^3

    explicit Mesh(const std::vector<Vertex>& vertices);
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void setInstances(const std::vector<InstanceData>& instances);
    void draw() const;

private:
    void release();

    GLuint vao_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint instanceBuffer_ = 0;
    GLsizei vertexCount_ = 0;
    GLsizei instanceCount_ = 0;
};
