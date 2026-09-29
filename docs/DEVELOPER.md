# Guía para desarrolladores

## Mapa de módulos

| Módulo | Responsabilidad | Ve OpenGL/GLFW |
|---|---|---|
| `Simulador` (+`Impl`) | API pública, máquina de estados Idle→Running→Closed, exclusión mutua de la API | no |
| `Board` | Tablero validado (`Cell` enum, robot localizado). Único punto de validación | no |
| `SharedBoard` | Mutex + copia del último tablero + contador de versión | no |
| `RenderLoop` | Hilo de render: crea Window/Renderer/Camera, bucle, arranque y parada | vía Window |
| `Window` | GLFW: init/terminate, ventana, contexto 3.3 Core, carga GLAD, entrada → `Input` | sí |
| `Camera` | Cámara orbital (yaw/pitch/distancia/objetivo), matrices View y Projection, encuadre | no (solo GLM) |
| `Renderer` | `Board` → instancias; animación del robot; uniforms y dibujo | sí |
| `Shader` | Compilación/enlace GLSL, RAII | sí |
| `Mesh` | VAO + VBO de vértices + VBO de instancias, RAII, movible | sí |

## Hilos y sincronización

* **Hilo del alumno** (cualquiera que llame a la API) y **hilo de render** (creado en `RenderLoop::start`).
* Único dato compartido mutable: `SharedBoard`. El alumno hace `publish` (swap bajo mutex); el render hace
  `fetchIfNewer` (comprueba versión y **copia** bajo mutex). El render nunca mantiene punteros al estado compartido.
  Secciones críticas de microsegundos.
* Flags atómicos: `stop_` (alumno→render) y `running_` (render→alumno).
* `Simulador::Impl::mutex` solo serializa la API (`publish/fin/esperarCierre`) entre hilos del usuario.
  **El hilo de render no lo toma jamás**, por lo que `fin()` puede mantenerlo mientras hace `join` sin riesgo de
  interbloqueo. Orden de bloqueo: `Impl::mutex` → `SharedBoard::mutex` (nunca al revés).
* Coalescencia: si se publica más rápido que el render, se pierden versiones intermedias, pero cada frame usa una
  versión completa.

## Ciclo de vida

```
Idle ──setTablero──▶ Running ──fin() / ventana cerrada──▶ Closed
  └───────────────────fin()────────────────────────────────▲
```
1. `setTablero` (en Idle): publica el tablero → `RenderLoop::start()` lanza el hilo y **espera una `std::promise`**
   que se cumple cuando ventana y recursos GL están listos. Si algo falla, la excepción viaja por el `future`
   hasta el llamante y el hilo se une.
2. El hilo de render itera: `pollInput` → obtener tablero nuevo → `Camera::update` → `Renderer::render` → swap.
3. Salida: `stop_` o `glfwWindowShouldClose`. Las variables locales se destruyen en orden inverso
   (Camera, **Renderer** —libera VAOs/VBOs/shader con el contexto vivo—, **Window** —suelta contexto, destruye la
   ventana, `glfwTerminate`—). Solo después `running_ = false`.
4. `fin()`: `requestStop()` + `join()`. Destructor: `fin()` en `try/catch`.

## Contexto OpenGL y GLFW

* El contexto se crea en el hilo de render (`Window` lo hace actual ahí) y **nadie más lo toca**. Todos los objetos
  GL nacen y mueren en ese hilo. Ningún otro hilo llama a GL.
* GLFW (init, ventanas, eventos) se usa solo desde el hilo de render. Esto funciona en Linux y Windows; en
  macOS Cocoa exige el hilo principal, por eso no está soportado.
* GLFW admite una instancia por proceso: `g_renderThreadActive` impide dos hilos de render simultáneos.
* Vsync (`glfwSwapInterval(1)`) marca ~60 FPS; hay un suelo de 4 ms por frame por si el vsync no frena.

## Representación del tablero

* Coordenadas de mundo: X derecha, Y arriba, Z hacia el observador. El tablero se centra en el origen; la celda
  `(x, y)` tiene el centro en `(x − W/2 + 0.5, 0, y − H/2 + 0.5)`. La fila 0 queda al fondo (arriba en pantalla).
* Cada casilla es un suelo delgado (0.94 × 0.94, deja rendija) con ajedrezado sutil. `-` y `R` usan el tono
  "visitada", `X` amarillo con una X roja en relieve, `*` un cubo oscuro de 0.9 de alto.
* Robot: 4 ruedas + cuerpo + cabeza (cubos) y una cuña (pirámide) como flecha de orientación.

### Rendimiento: instanced rendering
Solo hay dos formas base (cubo y pirámide). Cada `Mesh` tiene un VBO de instancias con `mat4 Model + vec4 color`
por elemento (atributos con divisor 1), así que todo el tablero estático es **una** llamada `glDrawArraysInstanced`
y el robot otras dos. La parte estática se regenera únicamente cuando llega un tablero nuevo; la del robot, solo
mientras se desliza. Un 256×256 son ~65 000 instancias (~5 MB).

## Shaders

`shaders/basic.vert|frag` (GLSL 330 core). Se embeben en `shaders_embedded.h` (generado en el paso de configuración
de CMake con raw strings): la biblioteca no busca ficheros en tiempo de ejecución. Cadena `Projection · View · Model`:
`Model` viene por instancia; `View` y `Projection` son uniforms del frame. Iluminación Blinn-Phong con una luz
direccional y ambiente hemisférico. Las normales usan `transpose(inverse(mat3(Model)))`.

## Cámara

Orbital: `posición = objetivo + d·(cosθ·sinφ, sinθ, cosθ·cosφ)` con φ = yaw y θ = pitch (limitado a 5°–89°).
`fitToBoard` calcula la distancia para que la esfera que envuelve el tablero quepa en el campo de visión más
estrecho (vertical u horizontal) y guarda esa pose como "inicial" (tecla `R`). Desplazar mueve el objetivo sobre
el suelo, con velocidad proporcional a la distancia (píxel → unidades de mundo reales).

## Ownership

Todo son valores/RAII: `std::unique_ptr<Impl>` en `Simulador`; `Mesh` y `Shader` liberan sus ids GL en el destructor
(`Mesh` es movible, `Shader` no); `Window` compone `GlfwLibrary` (init/terminate) + `unique_ptr<GLFWwindow>` con
deleter. No hay `new`/`delete` manuales.

## Tests

`ctest`: `test_board` (validación y parseo), `test_camera` (encuadre, zoom, reset, límites) y `test_simulador_api`
(errores y estados sin abrir ventana; sirve en CI sin pantalla). La parte con ventana se comprueba a mano con los ejemplos.
