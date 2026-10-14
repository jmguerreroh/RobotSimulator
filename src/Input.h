// Input.h - Entrada del usuario ya "digerida" por la ventana.
// Permite que Camera no dependa de GLFW.
#pragma once

#include <glm/vec2.hpp>

struct Input {
    glm::vec2 cursorDelta{0.0f};  // movimiento del ratón desde el frame anterior (píxeles)
    bool orbiting = false;        // botón izquierdo pulsado
    bool panning = false;         // botón derecho o central pulsado
    float scroll = 0.0f;          // pasos de rueda acumulados (+ = acercar)
    glm::vec2 keyMove{0.0f};      // teclado: x = derecha(+)/izquierda(-), y = adelante(+)/atrás(-)
    float keyRotate = 0.0f;       // teclado: +1 / -1 giro alrededor del tablero
    float keyZoom = 0.0f;         // teclado: +1 acercar / -1 alejar
    bool resetRequested = false;  // tecla R
};
