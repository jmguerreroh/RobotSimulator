# RobotSimulator — visor 3D de tableros para Robótica

Biblioteca C++17 (OpenGL 3.3 Core + GLFW + GLAD + GLM) que **dibuja en 3D, en tiempo real, el estado de un
tablero** de un robot. **No** calcula caminos ni toma decisiones: solo muestra lo que tu programa le cuenta.
La ventana tiene su propio hilo, así que **tu programa nunca se queda esperando a que se dibuje**.

## Para alumnos

### Empezar en 3 pasos

Solo necesitas una carpeta con tu `main.cpp` y un fichero `CMakeLists.txt`. **No hace falta descargar nada a mano**:
CMake trae el simulador de GitHub él solo.

**Paso 1 — Instala las herramientas (una sola vez).**

| Sistema | Comando |
|---|---|
| Ubuntu / Debian | `sudo apt install build-essential cmake git libglfw3-dev xorg-dev` |
| Fedora | `sudo dnf install gcc-c++ cmake git glfw-devel mesa-libGL-devel` |
| Windows | Visual Studio con «Desarrollo para el escritorio con C++» (incluye CMake) y [Git](https://git-scm.com) |

**Paso 2 — Crea estos dos ficheros en tu carpeta.**

`CMakeLists.txt` (cópialo tal cual, solo cambia `MiPrograma` si quieres):

```cmake
cmake_minimum_required(VERSION 3.16)
project(MiPrograma CXX)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(RobotSimulator
  GIT_REPOSITORY https://github.com/jmguerreroh/RobotSimulator.git
  GIT_TAG main)
FetchContent_MakeAvailable(RobotSimulator)

add_executable(MiPrograma main.cpp)
target_link_libraries(MiPrograma PRIVATE RobotSimulator)
```

`main.cpp` (tu programa; empieza con este):

```cpp
#include <Simulador.h>

int main()
{
    Simulador simulador;

    char tablero[5][8] = {
        {'R','.','.','.','.','.','.','.'},
        {'.','.','*','*','*','.','.','.'},
        {'.','.','.','.','*','.','.','.'},
        {'.','.','.','.','.','.','X','.'},
        {'.','.','.','.','.','.','.','.'},
    };

    simulador.setTablero(tablero);   // abre la ventana; el programa sigue

    // ... tu algoritmo: cambia el tablero y vuelve a llamar a setTablero(tablero) ...

    simulador.esperarCierre();       // mantiene la ventana hasta que la cierres
    simulador.fin();
}
```

**Paso 3 — Compila y ejecuta** (desde tu carpeta; la primera vez descarga el simulador y tarda un poco):

```bash
cmake -S . -B build
cmake --build build
./build/MiPrograma
```

En Windows, con Visual Studio: `cmake -S . -B build` y `cmake --build build --config Release`, y se ejecuta
`build\Release\MiPrograma.exe`. Cada vez que modifiques `main.cpp` solo repites los dos últimos comandos.

> Si ves un error de GLFW o de X11 al configurar, te falta instalar algo del paso 1.

### Cómo se usa

Cada vez que **cambie el tablero**, llama otra vez a `simulador.setTablero(tablero)`. Solo eso.

| Carácter | Significa |
|---|---|
| `R` | Robot (como mucho uno) |
| `X` | Objetivo |
| `*` | Obstáculo |
| `.` | Casilla libre |
| `-` | Casilla ya visitada |

Si algo está mal (carácter raro, tamaño 0…), `setTablero` lanza `std::invalid_argument` con un mensaje que
dice en qué fila y columna está el problema.

Ver `examples/` — `ejemplo_basico.cpp` (lo mínimo), `ejemplo_movimiento.cpp` (un algoritmo BFS que mueve al robot)
y `ejemplo_no_bloqueante.cpp`.

## Requisitos

* Compilador C++17 (GCC ≥ 8, Clang ≥ 8, MSVC 2019+), CMake ≥ 3.16.
* Una tarjeta gráfica/driver con **OpenGL 3.3**.
* GLFW 3.3+. GLM y GLAD **ya vienen incluidos** en `third_party/`.

### Instalación de dependencias

| Sistema | Comando |
|---|---|
| Ubuntu / Debian | `sudo apt install build-essential cmake libglfw3-dev libgl-dev xorg-dev` |
| Fedora | `sudo dnf install gcc-c++ cmake glfw-devel mesa-libGL-devel` |
| Windows (vcpkg) | `vcpkg install glfw3` y `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake` |
| Otros / sin GLFW | CMake lo descarga solo (`FetchContent`); necesita `git` y conexión |

> **macOS:** GLFW exige que las ventanas se creen en el hilo principal, y este simulador crea la ventana en un
> hilo propio. No está soportado en macOS. Funciona en Linux y Windows.

## Compilación

```bash
mkdir build && cd build
cmake ..
cmake --build .
ctest                              # (opcional) tests
./examples/ejemplo_movimiento      # en Windows: examples\Debug\ejemplo_movimiento.exe
```

Opciones: `-DROBOTSIM_BUILD_EXAMPLES=OFF`, `-DROBOTSIM_BUILD_TESTS=OFF`.

### Usarlo en tu propio proyecto (sin descarga automática)

Si prefieres clonar el repositorio tú mismo (`git clone https://github.com/TU_USUARIO/RobotSimulator.git`)
en lugar de usar `FetchContent` (ver «Empezar en 3 pasos»):

```cmake
add_subdirectory(RobotSimulator)                       # carpeta de este repositorio
add_executable(MiPrograma main.cpp)
target_link_libraries(MiPrograma PRIVATE RobotSimulator)
```
```cpp
#include <Simulador.h>
```

## Controles

| Acción | Control |
|---|---|
| Girar alrededor del tablero | botón izquierdo + arrastrar (o `Q` / `E`) |
| Desplazar la cámara | botón derecho (o central) + arrastrar (o flechas / `WASD`) |
| Zoom | rueda (o `+` / `-`) |
| Volver a la vista inicial | `R` |
| Cerrar | `Esc` o cerrar la ventana |

## Cerrar el simulador

* Cierra la ventana con la `X` de la ventana o `Esc`; el programa **no** termina, `setTablero` pasa a no hacer nada
  y `abierto()` devuelve `false` (úsalo para acabar tu bucle).
* `fin()` cierra la ventana desde el código. Puedes llamarla varias veces. Si te olvidas, el destructor la llama.

## API

Resumen; el detalle y las decisiones (llamadas repetidas, después de `fin()`…) están en [docs/API.md](docs/API.md).

```cpp
class Simulador {
    void setTablero(const char (&t)[Y][X]);                   // char t[Y][X]
    void setTablero(const char (*t)[N], int tamX, int tamY);  // char t[Y][X] con tamaño explícito
    void setTablero(const char* t, int tamX, int tamY);       // array plano por filas
    void setTablero(const char* const* filas, int tamX, int tamY); // char** / array de filas
    void setTablero(const std::vector<std::string>& filas);
    void fin();
    bool abierto() const;
    void esperarCierre();
};
```

## Arquitectura (resumen)

```
Simulador (API pública, pimpl)
  └─ Impl ── SharedBoard (mutex + copia del tablero + versión)
        └─ RenderLoop (hilo de render)
              ├─ Window    GLFW + contexto OpenGL + entrada
              ├─ Camera    cámara orbital
              └─ Renderer  Board → instancias
                    ├─ Shader  (basic.vert / basic.frag, embebidos)
                    └─ Mesh    cubo / pirámide + instanced rendering
Board  → tablero validado (Cell enum)
```

Más detalle para desarrolladores en [docs/DEVELOPER.md](docs/DEVELOPER.md).

## Estructura

```
CMakeLists.txt  README.md  LICENSE
include/Simulador.h        única cabecera pública
src/                       implementación (Board, Camera, Window, Shader, Mesh, Renderer, RenderLoop, SharedBoard)
shaders/                   basic.vert, basic.frag (se embeben en la biblioteca al configurar)
cmake/                     plantilla de la cabecera de shaders embebidos
examples/  tests/  docs/  third_party/ (GLAD, GLM)
```
