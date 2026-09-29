// Camera.h - Cámara orbital en perspectiva.
//
// La cámara mira siempre a un punto (target_) y se sitúa sobre una esfera a su
// alrededor, definida por yaw (giro), pitch (elevación) y distance.
// Mundo: X hacia la derecha, Y hacia arriba, Z hacia el observador.
#pragma once

#include <glm/glm.hpp>

#include "Input.h"

class Camera {
public:
    Camera();

    // Encuadra un tablero de width x height casillas (centrado en el origen) y
    // guarda esa vista como "vista inicial" (tecla R).
    void fitToBoard(int width, int height);
    void reset();

    void setViewport(int widthPx, int heightPx);
    void update(const Input& input, float dt);

    glm::mat4 view() const;
    glm::mat4 projection() const;
    glm::vec3 position() const;
    float distance() const { return distance_; }
    float pitch() const { return pitch_; }

private:
    struct Pose {
        glm::vec3 target{0.0f};
        float yaw = 0.0f;
        float pitch = 0.0f;
        float distance = 20.0f;
    };

    void applyPose(const Pose& p);
    void pan(const glm::vec2& screenDelta);
    void clampDistance();

    glm::vec3 target_{0.0f};
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float distance_ = 20.0f;
    Pose home_;
    float aspect_ = 4.0f / 3.0f;
    int viewportHeightPx_ = 768;
    float boardRadius_ = 10.0f;
};
