# API de `Simulador`

```cpp
#include <Simulador.h>
```

## Constructor / destructor
`Simulador()` no abre nada. `~Simulador()` equivale a `fin()`.
La clase no se puede copiar ni mover. Solo puede haber **un simulador con ventana abierta a la vez** por programa
(un segundo `setTablero` con la primera ventana aún abierta lanza `std::runtime_error`).

## `setTablero(...)`
Variantes (todas equivalentes; `X` = columnas, `Y` = filas, el carácter `(x, y)` es `tablero[y][x]`):

| Firma | Uso |
|---|---|
| `setTablero(const char (&)[Y][X])` | `char t[Y][X]; sim.setTablero(t);` |
| `setTablero(const char (*)[N], int tamX, int tamY)` | `sim.setTablero(t, X, Y);` (`tamX ≤ N`) |
| `setTablero(const char*, int tamX, int tamY)` | array plano: `t[y*tamX + x]` |
| `setTablero(const char* const*, int tamX, int tamY)` | `char**` / array de punteros a fila |
| `setTablero(const std::vector<std::string>&)` | filas de igual longitud |

Comportamiento:

* **Copia** el tablero: el puntero puede dejar de ser válido al volver.
* **No bloquea.** Solo valida, copia bajo un mutex y vuelve. La **primera** llamada además abre la ventana y espera
  a que exista (típicamente decenas de milisegundos).
* **Llamadas repetidas:** cada una sustituye el estado anterior; la ventana no se recrea. Si se llama más rápido
  que ~60 veces/s, se dibuja siempre el último tablero recibido (los intermedios se omiten, nunca se ve un
  tablero a medias).
* **Cambio de tamaño:** la cámara se reencuadra sola solo si cambian las dimensiones (si no, se respeta la vista
  del usuario).
* **Orientación del robot:** se deduce del último movimiento; el robot se desliza suavemente a su nueva casilla.
* **Errores** (`std::invalid_argument`, siempre antes de abrir nada y aunque el simulador ya esté cerrado):
  puntero nulo, `tamX`/`tamY` ≤ 0 o > 256, carácter fuera de `. - * R X` (el mensaje da fila y columna), más de una
  `R`, filas desiguales. Un tablero sin `R` es válido.
* Otros errores: `std::runtime_error` si no se puede abrir la ventana (sin pantalla, sin OpenGL 3.3…) o ya hay otra abierta.

## `fin()`
Pide parar al hilo de render, espera a que termine (ventana destruida, VAOs/VBOs/shaders liberados, GLFW terminado)
y deja el objeto cerrado. Es **idempotente**. Llamarla antes de cualquier `setTablero` también es válido.

## Estados y llamadas "raras"

| Situación | Resultado |
|---|---|
| `fin(); fin();` | seguro, la segunda no hace nada |
| `fin(); setTablero(...)` | se valida el tablero y **se ignora**; el simulador no se reabre |
| El usuario cierra la ventana y sigues llamando a `setTablero` | igual que arriba: se ignora sin error; `abierto()` es `false` |
| Olvidar `fin()` | el destructor la ejecuta |
| Terminar `main` sin `esperarCierre()` | la ventana se cierra al terminar el programa |

Por qué "cerrado es definitivo": si reabriera la ventana tras `fin()` o tras cerrarla el usuario, un bucle del
alumno la volvería a abrir en cada iteración. Un objeto nuevo (`Simulador otro;`) sí abre una ventana nueva
una vez cerrada la anterior.

## `bool abierto() const`
`true` antes de la primera llamada a `setTablero` y mientras la ventana esté abierta; `false` después.

## `void pause(int ms)`
Espera `ms` milisegundos para que dé tiempo a ver el tablero; la ventana sigue dibujando durante la espera. Si la
ventana se cierra durante la pausa (o ya estaba cerrada, o se llamó a `fin()`) vuelve **de inmediato**, así un bucle con
`pause` termina enseguida cuando el usuario cierra la ventana. `ms == 0` no espera; `ms < 0` lanza `std::invalid_argument`.
Funciona también antes de abrir la ventana. Equivale a un `sleep` interrumpible: no sustituye a `setTablero`.

## `void esperarCierre()`
Bloquea el hilo llamante hasta que el usuario cierre la ventana (o `fin()` se llame desde otro hilo). Si no hay
ventana abierta, vuelve al instante.

## Hilos
`setTablero`, `fin`, `abierto`, `pause` y `esperarCierre` pueden llamarse desde cualquier hilo. No hay callbacks al código del alumno.
