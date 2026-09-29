// Simulador.h - Única cabecera que necesita conocer el alumno.
//
// Muestra en una ventana 3D el estado de un tablero de caracteres:
//
//     R  robot          X  objetivo        *  obstáculo
//     .  casilla libre  -  casilla visitada
//
// Ejemplo mínimo:
//
//     #include <Simulador.h>
//
//     int main() {
//         Simulador simulador;
//         char tablero[3][5] = { {'R','.','.','*','.'},
//                                {'.','.','.','*','.'},
//                                {'.','.','.','.','X'} };
//         simulador.setTablero(tablero);   // abre la ventana; NO bloquea
//         // ... algoritmo del alumno, llamando a setTablero() cuando cambie ...
//         simulador.esperarCierre();       // (opcional) espera a que se cierre la ventana
//         simulador.fin();
//     }
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class Simulador {
public:
    Simulador();
    ~Simulador();  // Equivale a llamar a fin(): nunca deja recursos abiertos.

    Simulador(const Simulador&) = delete;
    Simulador& operator=(const Simulador&) = delete;
    Simulador(Simulador&&) = delete;
    Simulador& operator=(Simulador&&) = delete;

    // ---- Actualizar el tablero -------------------------------------------
    // Todas las variantes copian el tablero (el puntero puede dejar de ser
    // válido justo después), abren la ventana la primera vez y vuelven de
    // inmediato. Lanzan std::invalid_argument si el tablero no es válido
    // (puntero nulo, tamaño <= 0 o > 256, carácter desconocido, más de una 'R').

    // Tablero plano por filas: el carácter (x, y) está en tablero[y * tamX + x].
    void setTablero(const char* tablero, int tamX, int tamY);

    // Tablero como array de punteros a filas (p. ej. char** creado con new).
    void setTablero(const char* const* filas, int tamX, int tamY);

    // Tablero como vector de cadenas; todas las filas deben medir lo mismo.
    void setTablero(const std::vector<std::string>& filas);

    // Array 2D de tamaño fijo: char tablero[Y][X]. El tamaño se deduce solo.
    template <std::size_t Y, std::size_t X>
    void setTablero(const char (&tablero)[Y][X]) {
        setTableroConPaso(&tablero[0][0], static_cast<int>(X), static_cast<int>(Y),
                          static_cast<int>(X));
    }

    // Array 2D de tamaño fijo con el tamaño explícito: setTablero(tablero, X, Y).
    // tamX no puede superar el número de columnas del array.
    template <std::size_t N>
    void setTablero(const char (*tablero)[N], int tamX, int tamY) {
        setTableroConPaso(&tablero[0][0], tamX, tamY, static_cast<int>(N));
    }

    // ---- Ciclo de vida ----------------------------------------------------
    // Cierra la ventana y libera todo. Se puede llamar varias veces.
    // Después de fin(), setTablero() valida el tablero pero no hace nada más.
    void fin();

    // false cuando la ventana se cerró (por el usuario o por fin()).
    // Antes de la primera llamada a setTablero() devuelve true.
    bool abierto() const;

    // Bloquea hasta que el usuario cierre la ventana. Si no hay ventana abierta
    // vuelve de inmediato. Útil al final de main() para poder contemplar el resultado.
    void esperarCierre();

private:
    void setTableroConPaso(const char* tablero, int tamX, int tamY, int paso);

    struct Impl;
    std::unique_ptr<Impl> impl_;
};
