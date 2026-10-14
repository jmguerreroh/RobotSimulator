#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float kFovY = glm::radians(45.0f);
constexpr float kHomePitch = glm::radians(55.0f);
constexpr float kMinPitch = glm::radians(5.0f);
constexpr float kMaxPitch = glm::radians(89.0f);
constexpr float kOrbitSpeed = 0.006f;      // rad por píxel
constexpr float kZoomStep = 0.9f;          // factor de distancia por paso de rueda
constexpr float kKeyPanSpeed = 0.6f;       // distancias por segundo
constexpr float kKeyRotateSpeed = 1.5f;    // rad por segundo
constexpr float kKeyZoomSpeed = 1.2f;      // e-foldings por segundo
constexpr float kMinDistance = 2.0f;
}  // namespace

Camera::Camera() { fitToBoard(10, 10); }

void Camera::fitToBoard(int width, int height) {
    // Esfera que envuelve el tablero (con margen para la altura de los obstáculos).
    boardRadius_ = 0.5f * std::hypot(static_cast<float>(width), static_cast<float>(height)) + 1.0f;
    // Semi-campo de visión más estrecho (vertical u horizontal): así cabe en cualquier ventana.
    const float halfV = kFovY * 0.5f;
    const float halfH = std::atan(std::tan(halfV) * aspect_);
    const float halfMin = std::min(halfV, halfH);

    home_.target = glm::vec3(0.0f);
    home_.yaw = 0.0f;
    home_.pitch = kHomePitch;
    home_.distance = boardRadius_ / std::sin(halfMin);
    reset();
}

void Camera::reset() { applyPose(home_); }

void Camera::applyPose(const Pose& p) {
    target_ = p.target;
    yaw_ = p.yaw;
    pitch_ = p.pitch;
    distance_ = p.distance;
}

void Camera::setViewport(int widthPx, int heightPx) {
    if (widthPx <= 0 || heightPx <= 0) return;
    aspect_ = static_cast<float>(widthPx) / static_cast<float>(heightPx);
    viewportHeightPx_ = heightPx;
}

void Camera::clampDistance() {
    distance_ = std::clamp(distance_, kMinDistance, std::max(kMinDistance, home_.distance * 8.0f));
}

void Camera::pan(const glm::vec2& d) {
    // Unidades de mundo por píxel a la distancia del objetivo.
    const float perPixel = 2.0f * distance_ * std::tan(kFovY * 0.5f) / static_cast<float>(viewportHeightPx_);
    const glm::vec3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));
    const glm::vec3 forward(-std::sin(yaw_), 0.0f, -std::cos(yaw_));  // hacia el fondo, sobre el suelo
    // La escena "sigue" al cursor: arrastrar a la derecha desplaza el objetivo a la izquierda.
    target_ -= right * (d.x * perPixel);
    target_ += forward * (d.y * perPixel / std::sin(pitch_));
}

void Camera::update(const Input& in, float dt) {
    if (in.resetRequested) {
        reset();
        return;
    }
    if (in.orbiting && !in.panning) {
        yaw_ -= in.cursorDelta.x * kOrbitSpeed;
        pitch_ = std::clamp(pitch_ + in.cursorDelta.y * kOrbitSpeed, kMinPitch, kMaxPitch);
    }
    if (in.panning) pan(in.cursorDelta);

    if (in.keyMove != glm::vec2(0.0f)) {
        const glm::vec3 right(std::cos(yaw_), 0.0f, -std::sin(yaw_));
        const glm::vec3 forward(-std::sin(yaw_), 0.0f, -std::cos(yaw_));
        target_ += (right * in.keyMove.x + forward * in.keyMove.y) * (kKeyPanSpeed * distance_ * dt);
    }
    yaw_ += in.keyRotate * kKeyRotateSpeed * dt;

    distance_ *= std::pow(kZoomStep, in.scroll);
    distance_ *= std::exp(-in.keyZoom * kKeyZoomSpeed * dt);
    clampDistance();
}

glm::vec3 Camera::position() const {
    return target_ + distance_ * glm::vec3(std::cos(pitch_) * std::sin(yaw_), std::sin(pitch_),
                                           std::cos(pitch_) * std::cos(yaw_));
}

glm::mat4 Camera::view() const { return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f)); }

glm::mat4 Camera::projection() const {
    const float far = home_.distance * 20.0f + 100.0f;
    return glm::perspective(kFovY, aspect_, 0.1f, far);
}
