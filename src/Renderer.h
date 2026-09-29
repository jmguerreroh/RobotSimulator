// Renderer.h - Traduce un Board a geometría OpenGL y lo dibuja.
//
// Requiere un contexto OpenGL actual en el hilo que lo usa (lo aporta Window).
//
// Estrategia de rendimiento:
//   * Parte ESTÁTICA (suelo, obstáculos, marcas de objetivo): se reconstruye y
//     sube a la GPU únicamente cuando llega un tablero nuevo.
//   * Parte DINÁMICA (robot, unas pocas cajas): se recalcula sólo mientras el
//     robot se desliza suavemente hacia su nueva casilla.
#pragma once

#include <glm/glm.hpp>

#include "Board.h"
#include "Camera.h"
#include "Mesh.h"
#include "Shader.h"

class Renderer {
public:
    Renderer();

    void setBoard(const Board& board);
    void render(const Camera& camera, const glm::ivec2& framebufferSize, float dt);

private:
    void rebuildStatic();
    void rebuildRobot();
    glm::vec2 cellCenter(int x, int y) const;

    Shader shader_;
    Mesh cube_;
    Mesh robotCubes_;
    Mesh robotWedge_;

    Board board_;
    bool robotVisible_ = false;
    glm::vec2 robotPos_{0.0f};     // posición mostrada (x, z en el mundo)
    glm::vec2 robotTarget_{0.0f};  // posición real según el tablero
    float robotHeading_;           // ángulo de giro sobre Y (rad)
    bool robotDirty_ = false;
};
