// Overlay.h - Rótulo de texto en 2D dibujado encima de la escena.
//
// Sin dependencias de fuentes: usa una fuente de mapa de bits de 5x7 píxeles
// (mayúsculas, cifras y unos pocos signos) y dibuja cada píxel encendido como
// un cuadrado. Pensado para mensajes cortos de estado ("esperando cierre"...).
//
// Requiere un contexto OpenGL actual en el hilo que lo usa (lo aporta Window).
#pragma once

#include <glad/glad.h>

#include <string>

#include <glm/glm.hpp>

#include "Shader.h"

class Overlay {
public:
    Overlay();
    ~Overlay();

    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;

    // Dibuja 'lineas' (separadas por '\n') centradas en la parte superior, sobre una banda oscura.
    void drawBanner(const std::string& lineas, const glm::ivec2& framebufferSize);

private:
    Shader shader_;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
};
