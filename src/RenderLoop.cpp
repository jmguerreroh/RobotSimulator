#include "RenderLoop.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "Board.h"
#include "Camera.h"
#include "Renderer.h"
#include "Window.h"

namespace {

using Clock = std::chrono::steady_clock;

// GLFW sólo admite una instancia por proceso: garantizamos un único hilo de render vivo.
std::atomic<bool> g_renderThreadActive{false};

constexpr int kWindowWidth = 1024;
constexpr int kWindowHeight = 768;
constexpr const char* kWindowTitle = "Simulador de Robot";
constexpr auto kMinFrameTime = std::chrono::milliseconds(4);  // red de seguridad si el vsync no frena (ventana minimizada, etc.)

}  // namespace

RenderLoop::~RenderLoop() {
    requestStop();
    join();
}

void RenderLoop::join() {
    if (thread_.joinable()) thread_.join();
}

void RenderLoop::start() {
    if (g_renderThreadActive.exchange(true)) {
        throw std::runtime_error("Simulador: ya hay otra ventana del simulador abierta en este programa");
    }
    stop_.store(false);
    running_.store(true);

    std::promise<void> ready;
    std::future<void> readyFuture = ready.get_future();
    try {
        thread_ = std::thread([this, &ready] { threadMain(ready); });
    } catch (...) {
        running_.store(false);
        g_renderThreadActive.store(false);
        throw;
    }

    try {
        readyFuture.get();  // espera a "ventana lista" (o relanza el error del hilo)
    } catch (...) {
        join();
        throw;
    }
}

void RenderLoop::threadMain(std::promise<void>& ready) {
    bool readySignalled = false;
    try {
        Window window(kWindowWidth, kWindowHeight, kWindowTitle);  // 1º: contexto GL
        Renderer renderer;                                         // 2º: recursos GL (se liberan antes que la ventana)
        Camera camera;

        ready.set_value();
        readySignalled = true;

        std::uint64_t seenVersion = 0;
        int boardWidth = 0, boardHeight = 0;
        auto previous = Clock::now();

        while (!stop_.load() && !window.shouldClose()) {
            const auto frameStart = Clock::now();
            const float dt = std::min(std::chrono::duration<float>(frameStart - previous).count(), 0.1f);
            previous = frameStart;

            const Input input = window.pollInput();
            const glm::ivec2 fb = window.framebufferSize();
            if (fb.x > 0 && fb.y > 0) {
                camera.setViewport(fb.x, fb.y);

                Board fresh;
                if (board_.fetchIfNewer(seenVersion, fresh)) {
                    if (fresh.width() != boardWidth || fresh.height() != boardHeight) {
                        boardWidth = fresh.width();
                        boardHeight = fresh.height();
                        camera.fitToBoard(boardWidth, boardHeight);  // sólo si cambian las dimensiones
                    }
                    renderer.setBoard(fresh);
                }

                camera.update(input, dt);
                renderer.render(camera, fb, dt);
            }
            window.swapBuffers();

            const auto elapsed = Clock::now() - frameStart;
            if (elapsed < kMinFrameTime) std::this_thread::sleep_for(kMinFrameTime - elapsed);
        }
    } catch (...) {
        if (!readySignalled) {
            ready.set_exception(std::current_exception());
        } else {
            try {
                throw;
            } catch (const std::exception& e) {
                std::cerr << "Simulador: el hilo de render termino por un error: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Simulador: el hilo de render termino por un error desconocido" << std::endl;
            }
        }
    }
    // Window, Renderer y Camera ya están destruidos: GL y GLFW liberados.
    running_.store(false);
    g_renderThreadActive.store(false);
}
