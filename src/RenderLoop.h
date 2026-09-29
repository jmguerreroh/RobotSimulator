// RenderLoop.h - Hilo de render: crea la ventana y la dibuja hasta que se le pide parar
// o el usuario la cierra.
//
// Propiedad de los recursos: TODO lo relacionado con OpenGL/GLFW (Window,
// Renderer, Camera) son variables locales de la función del hilo. Se crean y
// se destruyen en ese hilo, en orden inverso (Renderer antes que Window), de
// modo que el contexto OpenGL siempre existe mientras se liberan VAOs/VBOs/shaders.
#pragma once

#include <atomic>
#include <future>
#include <thread>

#include "SharedBoard.h"

class RenderLoop {
public:
    explicit RenderLoop(const SharedBoard& board) : board_(board) {}
    ~RenderLoop();  // pide parada y espera al hilo

    RenderLoop(const RenderLoop&) = delete;
    RenderLoop& operator=(const RenderLoop&) = delete;

    // Lanza el hilo y espera a que la ventana esté creada. Si falla (sin
    // pantalla, sin OpenGL 3.3, otra ventana activa...) lanza std::runtime_error.
    void start();
    void requestStop() { stop_.store(true); }
    void join();

    // true mientras el hilo de render siga vivo. Pasa a false cuando ya se
    // liberaron ventana, recursos OpenGL y GLFW.
    bool running() const { return running_.load(); }

private:
    void threadMain(std::promise<void>& ready);

    const SharedBoard& board_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> running_{false};
};
