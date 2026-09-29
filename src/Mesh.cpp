#include "Mesh.h"

#include <cstddef>
#include <utility>

namespace {

// Añade un triángulo cuya normal apunta hacia fuera del sólido: 'inside' es un
// punto interior y, si el triángulo mira hacia él, se invierte el orden de los
// vértices. Así el sentido antihorario (necesario para culling) es siempre correcto.
void addTriangle(std::vector<Vertex>& out, glm::vec3 a, glm::vec3 b, glm::vec3 c, const glm::vec3& inside) {
    glm::vec3 n = glm::cross(b - a, c - a);
    if (glm::dot(n, a - inside) < 0.0f) {
        std::swap(b, c);
        n = -n;
    }
    n = glm::normalize(n);
    out.push_back({a, n});
    out.push_back({b, n});
    out.push_back({c, n});
}

void addQuad(std::vector<Vertex>& out, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c,
             const glm::vec3& d, const glm::vec3& inside) {
    addTriangle(out, a, b, c, inside);
    addTriangle(out, a, c, d, inside);
}

}  // namespace

Mesh Mesh::cube() {
    std::vector<Vertex> v;
    const glm::vec3 origin(0.0f);
    for (int axis = 0; axis < 3; ++axis) {
        glm::vec3 n(0.0f), u(0.0f), w(0.0f);
        u[(axis + 1) % 3] = 0.5f;
        w[(axis + 2) % 3] = 0.5f;
        for (float sign : {-1.0f, 1.0f}) {
            n = glm::vec3(0.0f);
            n[axis] = 0.5f * sign;
            addQuad(v, n - u - w, n + u - w, n + u + w, n - u + w, origin);
        }
    }
    return Mesh(v);
}

Mesh Mesh::pyramid() {
    std::vector<Vertex> v;
    const glm::vec3 inside(-0.2f, 0.0f, 0.0f);
    const glm::vec3 tip(0.5f, 0.0f, 0.0f);
    const glm::vec3 b0(-0.5f, -0.5f, -0.5f), b1(-0.5f, -0.5f, 0.5f), b2(-0.5f, 0.5f, 0.5f), b3(-0.5f, 0.5f, -0.5f);
    addTriangle(v, tip, b0, b1, inside);
    addTriangle(v, tip, b1, b2, inside);
    addTriangle(v, tip, b2, b3, inside);
    addTriangle(v, tip, b3, b0, inside);
    addQuad(v, b0, b1, b2, b3, inside);  // base
    return Mesh(v);
}

Mesh::Mesh(const std::vector<Vertex>& vertices) : vertexCount_(static_cast<GLsizei>(vertices.size())) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &instanceBuffer_);

    glBindVertexArray(vao_);

    // Atributos por vértice: location 0 = posición, 1 = normal.
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));

    // Atributos por instancia: locations 2..5 = matriz Model (4 columnas), 6 = color.
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    for (int col = 0; col < 4; ++col) {
        const GLuint loc = 2 + static_cast<GLuint>(col);
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              reinterpret_cast<void*>(offsetof(InstanceData, model) + sizeof(glm::vec4) * col));
        glVertexAttribDivisor(loc, 1);
    }
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), reinterpret_cast<void*>(offsetof(InstanceData, color)));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Mesh::~Mesh() { release(); }

Mesh::Mesh(Mesh&& o) noexcept
    : vao_(std::exchange(o.vao_, 0)),
      vertexBuffer_(std::exchange(o.vertexBuffer_, 0)),
      instanceBuffer_(std::exchange(o.instanceBuffer_, 0)),
      vertexCount_(std::exchange(o.vertexCount_, 0)),
      instanceCount_(std::exchange(o.instanceCount_, 0)) {}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        release();
        vao_ = std::exchange(o.vao_, 0);
        vertexBuffer_ = std::exchange(o.vertexBuffer_, 0);
        instanceBuffer_ = std::exchange(o.instanceBuffer_, 0);
        vertexCount_ = std::exchange(o.vertexCount_, 0);
        instanceCount_ = std::exchange(o.instanceCount_, 0);
    }
    return *this;
}

void Mesh::release() {
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
    if (vertexBuffer_ != 0) glDeleteBuffers(1, &vertexBuffer_);
    if (instanceBuffer_ != 0) glDeleteBuffers(1, &instanceBuffer_);
    vao_ = vertexBuffer_ = instanceBuffer_ = 0;
}

void Mesh::setInstances(const std::vector<InstanceData>& instances) {
    instanceCount_ = static_cast<GLsizei>(instances.size());
    glBindBuffer(GL_ARRAY_BUFFER, instanceBuffer_);
    // Con nullptr + tamaño se "huérfana" el buffer viejo, evitando esperar a la GPU.
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(InstanceData)),
                 instances.empty() ? nullptr : instances.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw() const {
    if (instanceCount_ == 0) return;
    glBindVertexArray(vao_);
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount_, instanceCount_);
    glBindVertexArray(0);
}
