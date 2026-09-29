// ejemplo_no_bloqueante.cpp - Demuestra que setTablero() NO bloquea.
//
// Actualizamos el tablero 2000 veces seguidas (mucho más rápido que los 60
// fotogramas por segundo de la ventana) y medimos cuánto tarda cada llamada.
// La ventana sigue dibujando y se puede mover la cámara mientras tanto.
#include <Simulador.h>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

int main() {
    using Reloj = std::chrono::steady_clock;

    const int X = 40, Y = 30;
    std::vector<std::string> tablero(Y, std::string(X, '.'));
    for (int y = 5; y < 25; ++y) tablero[y][20] = '*';
    tablero[15][35] = 'X';
    tablero[15][2] = 'R';

    Simulador simulador;
    simulador.setTablero(tablero);  // la primera llamada incluye abrir la ventana

    const auto inicio = Reloj::now();
    auto peor = Reloj::duration::zero();
    int x = 2, y = 15;
    for (int i = 0; i < 2000; ++i) {
        // "Algoritmo": el robot recorre el tablero dejando rastro.
        tablero[y][x] = '-';
        x = 2 + (i % 33);
        y = 15 + ((i / 33) % 2 ? 1 : -1) * ((i / 66) % 8);
        if (tablero[y][x] == '*') x++;
        tablero[y][x] = 'R';

        const auto t0 = Reloj::now();
        simulador.setTablero(tablero);  // vuelve inmediatamente
        peor = std::max(peor, Reloj::now() - t0);
    }
    const auto total = std::chrono::duration<double, std::milli>(Reloj::now() - inicio).count();

    std::cout << "2000 actualizaciones en " << total << " ms\n"
              << "La llamada a setTablero() mas lenta tardo "
              << std::chrono::duration<double, std::micro>(peor).count() << " us\n"
              << "Cierra la ventana para terminar.\n";

    simulador.esperarCierre();
    simulador.fin();
    return 0;
}
