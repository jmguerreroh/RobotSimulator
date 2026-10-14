#include <cmath>

#include <glm/glm.hpp>

#include "Camera.h"
#include "check.h"

int main() {
    Camera small, large;
    small.fitToBoard(5, 5);
    large.fitToBoard(50, 50);
    CHECK(large.distance() > small.distance());
    CHECK(small.position().y > 0.0f);  // por encima del tablero

    // Todas las esquinas del tablero 50x50 quedan dentro del cuadro visible.
    const glm::mat4 vp = large.projection() * large.view();
    for (float sx : {-25.0f, 25.0f})
        for (float sz : {-25.0f, 25.0f}) {
            const glm::vec4 clip = vp * glm::vec4(sx, 0.0f, sz, 1.0f);
            CHECK(clip.w > 0.0f);
            CHECK(std::abs(clip.x / clip.w) <= 1.0f && std::abs(clip.y / clip.w) <= 1.0f);
        }

    // Zoom con la rueda acerca; reset devuelve a la vista inicial.
    const float home = large.distance();
    Input zoom;
    zoom.scroll = 3.0f;
    large.update(zoom, 0.016f);
    CHECK(large.distance() < home);
    Input reset;
    reset.resetRequested = true;
    large.update(reset, 0.016f);
    CHECK(std::abs(large.distance() - home) < 1e-4f);

    // La elevación se limita para no dar la vuelta por encima del polo.
    Input orbit;
    orbit.orbiting = true;
    orbit.cursorDelta = {0.0f, 100000.0f};
    large.update(orbit, 0.016f);
    CHECK(large.pitch() < glm::radians(90.0f));
    return 0;
}
