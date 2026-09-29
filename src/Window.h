// Window.h - Ventana GLFW + contexto OpenGL 3.3 Core.
//
// Toda la vida de la ventana (creación, eventos, destrucción) debe ocurrir en
// UN solo hilo: el de render. El constructor inicializa GLFW, crea la ventana,
// hace el contexto actual en el hilo que lo llama y carga las funciones de
// OpenGL con GLAD. El destructor lo deshace todo en orden inverso.
#pragma once

#include <memory>

#include <glm/vec2.hpp>

#include "Input.h"

struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const char* title);  // lanza std::runtime_error si falla
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    Input pollInput();  // procesa eventos y devuelve el estado de entrada de este frame
    glm::ivec2 framebufferSize() const;

private:
    struct GlfwLibrary {  // RAII de glfwInit / glfwTerminate
        GlfwLibrary();
        ~GlfwLibrary();
    };
    struct WindowDeleter {
        void operator()(GLFWwindow* w) const;
    };

    static void onScroll(GLFWwindow* w, double dx, double dy);
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);

    GlfwLibrary glfw_;  // se declara antes que window_: se destruye después
    std::unique_ptr<GLFWwindow, WindowDeleter> window_;

    bool haveCursor_ = false;
    double lastX_ = 0.0;
    double lastY_ = 0.0;
    float scroll_ = 0.0f;
    bool resetPressed_ = false;
};
