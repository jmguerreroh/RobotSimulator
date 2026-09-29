#include "Renderer.h"

#include <glad/glad.h>

#include <cmath>
#include <vector>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shaders_embedded.h"

namespace {

// Paleta: pensada para distinguir cada tipo de casilla de un vistazo.
const glm::vec4 kFreeColor{0.80f, 0.81f, 0.78f, 1.0f};
const glm::vec4 kVisitedColor{0.45f, 0.72f, 0.95f, 1.0f};
const glm::vec4 kGoalColor{0.98f, 0.82f, 0.15f, 1.0f};
const glm::vec4 kGoalMarkColor{0.85f, 0.10f, 0.12f, 1.0f};
const glm::vec4 kObstacleColor{0.27f, 0.29f, 0.34f, 1.0f};
const glm::vec4 kRobotBodyColor{0.95f, 0.45f, 0.08f, 1.0f};
const glm::vec4 kRobotHeadColor{0.92f, 0.94f, 0.96f, 1.0f};
const glm::vec4 kRobotWheelColor{0.10f, 0.10f, 0.12f, 1.0f};
const glm::vec4 kRobotArrowColor{0.10f, 0.55f, 0.90f, 1.0f};
const glm::vec3 kClearColor{0.09f, 0.10f, 0.14f};

constexpr float kTileSize = 0.94f;       // < 1: deja una rendija visible entre casillas
constexpr float kTileThickness = 0.10f;
constexpr float kObstacleHeight = 0.9f;
constexpr float kRobotSmoothing = 18.0f;  // 1/s: mayor = movimiento más brusco

// Model = T * R * S
glm::mat4 makeModel(const glm::vec3& position, float yaw, const glm::vec3& scale) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), position);
    m = glm::rotate(m, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::scale(m, scale);
}

}  // namespace

Renderer::Renderer()
    : shader_(shaders::kBasicVert, shaders::kBasicFrag),
      cube_(Mesh::cube()),
      robotCubes_(Mesh::cube()),
      robotWedge_(Mesh::pyramid()),
      robotHeading_(-glm::half_pi<float>()) {  // mirando hacia +Z (hacia el observador)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
}

glm::vec2 Renderer::cellCenter(int x, int y) const {
    return {static_cast<float>(x) - 0.5f * board_.width() + 0.5f, static_cast<float>(y) - 0.5f * board_.height() + 0.5f};
}

void Renderer::setBoard(const Board& board) {
    const bool sameShape = board.width() == board_.width() && board.height() == board_.height();
    board_ = board;
    rebuildStatic();

    if (!board_.hasRobot()) {
        robotVisible_ = false;
        robotCubes_.setInstances({});
        robotWedge_.setInstances({});
        return;
    }
    const glm::vec2 newTarget = cellCenter(board_.robotX(), board_.robotY());
    if (!robotVisible_ || !sameShape) {
        robotPos_ = newTarget;  // primera aparición (o tablero nuevo): sin deslizamiento
    } else if (newTarget != robotTarget_) {
        const glm::vec2 delta = newTarget - robotTarget_;
        robotHeading_ = std::atan2(-delta.y, delta.x);  // orientación = sentido del último movimiento
    }
    robotTarget_ = newTarget;
    robotVisible_ = true;
    robotDirty_ = true;
}

void Renderer::rebuildStatic() {
    std::vector<InstanceData> inst;
    inst.reserve(static_cast<std::size_t>(board_.width()) * board_.height() + 16);

    for (int y = 0; y < board_.height(); ++y) {
        for (int x = 0; x < board_.width(); ++x) {
            const glm::vec2 c = cellCenter(x, y);
            const Cell cell = board_.at(x, y);
            const float checker = ((x + y) & 1) ? 0.93f : 1.0f;  // ajedrezado sutil: ayuda a contar casillas

            glm::vec4 floor = kFreeColor;
            if (cell == Cell::Visited || cell == Cell::Robot) floor = kVisitedColor;  // bajo el robot: ya visitada
            if (cell == Cell::Goal) floor = kGoalColor;
            if (cell == Cell::Obstacle) floor = kFreeColor * 0.7f;
            if (cell != Cell::Goal) floor = glm::vec4(glm::vec3(floor) * checker, 1.0f);

            inst.push_back({makeModel({c.x, -0.5f * kTileThickness, c.y}, 0.0f, {kTileSize, kTileThickness, kTileSize}), floor});

            if (cell == Cell::Obstacle) {
                inst.push_back({makeModel({c.x, 0.5f * kObstacleHeight, c.y}, 0.0f, {0.9f, kObstacleHeight, 0.9f}),
                                kObstacleColor * (0.9f + 0.1f * checker)});
            } else if (cell == Cell::Goal) {
                // Una "X" en relieve formada por dos barras cruzadas.
                for (float angle : {glm::quarter_pi<float>(), -glm::quarter_pi<float>()}) {
                    inst.push_back({makeModel({c.x, 0.5f * kTileThickness + 0.05f, c.y}, angle, {0.78f, 0.10f, 0.14f}),
                                    kGoalMarkColor});
                }
            }
        }
    }
    cube_.setInstances(inst);
}

void Renderer::rebuildRobot() {
    const glm::vec3 base(robotPos_.x, 0.0f, robotPos_.y);
    const glm::mat4 frame = glm::translate(glm::mat4(1.0f), base) *
                            glm::rotate(glm::mat4(1.0f), robotHeading_, glm::vec3(0.0f, 1.0f, 0.0f));
    // Cada pieza se define en el sistema local del robot (+X = delante) y se lleva al mundo con 'frame'.
    auto part = [&](const glm::vec3& center, const glm::vec3& size, const glm::vec4& color) {
        return InstanceData{frame * glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), size), color};
    };

    std::vector<InstanceData> boxes;
    for (float sx : {-0.2f, 0.2f}) {
        for (float sz : {-0.28f, 0.28f}) boxes.push_back(part({sx, 0.09f, sz}, {0.20f, 0.18f, 0.10f}, kRobotWheelColor));
    }
    boxes.push_back(part({0.0f, 0.21f, 0.0f}, {0.64f, 0.20f, 0.46f}, kRobotBodyColor));    // cuerpo
    boxes.push_back(part({-0.08f, 0.40f, 0.0f}, {0.30f, 0.18f, 0.30f}, kRobotHeadColor));  // cabeza
    robotCubes_.setInstances(boxes);

    // Flecha de orientación sobre el capó.
    robotWedge_.setInstances({part({0.20f, 0.33f, 0.0f}, {0.26f, 0.05f, 0.22f}, kRobotArrowColor)});
}

void Renderer::render(const Camera& camera, const glm::ivec2& fb, float dt) {
    if (robotVisible_) {
        const glm::vec2 d = robotTarget_ - robotPos_;
        if (glm::dot(d, d) > 1e-6f) {
            robotPos_ += d * (1.0f - std::exp(-kRobotSmoothing * dt));
            if (glm::dot(robotTarget_ - robotPos_, robotTarget_ - robotPos_) < 1e-6f) robotPos_ = robotTarget_;
            robotDirty_ = true;
        }
        if (robotDirty_) {
            rebuildRobot();
            robotDirty_ = false;
        }
    }

    glViewport(0, 0, fb.x, fb.y);
    glClearColor(kClearColor.r, kClearColor.g, kClearColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader_.use();
    shader_.setMat4("uView", camera.view());
    shader_.setMat4("uProjection", camera.projection());
    shader_.setVec3("uViewPos", camera.position());
    shader_.setVec3("uLightDir", glm::normalize(glm::vec3(0.4f, 1.0f, 0.6f)));

    cube_.draw();
    if (robotVisible_) {
        robotCubes_.draw();
        robotWedge_.draw();
    }
}
