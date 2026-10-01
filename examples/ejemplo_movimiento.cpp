// ejemplo_movimiento.cpp - Un "algoritmo del alumno" (búsqueda en anchura) que
// lleva al robot hasta la X y va actualizando el simulador paso a paso.
//
// Fíjate en que el simulador NO decide nada: sólo muestra lo que el programa le cuenta.
#include <Simulador.h>

#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using Tablero = std::vector<std::string>;

// Devuelve el camino (lista de casillas x,y) desde 'R' hasta 'X', o vacío si no existe.
static std::vector<std::pair<int, int>> buscarCamino(const Tablero& t) {
    const int alto = static_cast<int>(t.size());
    const int ancho = static_cast<int>(t[0].size());
    std::pair<int, int> inicio{-1, -1}, meta{-1, -1};
    for (int y = 0; y < alto; ++y)
        for (int x = 0; x < ancho; ++x) {
            if (t[y][x] == 'R') inicio = {x, y};
            if (t[y][x] == 'X') meta = {x, y};
        }
    if (inicio.first < 0 || meta.first < 0) return {};

    std::vector<std::vector<std::pair<int, int>>> padre(alto, std::vector<std::pair<int, int>>(ancho, {-2, -2}));
    std::queue<std::pair<int, int>> cola;
    cola.push(inicio);
    padre[inicio.second][inicio.first] = inicio;
    const int dx[] = {1, -1, 0, 0}, dy[] = {0, 0, 1, -1};
    while (!cola.empty()) {
        auto [x, y] = cola.front();
        cola.pop();
        if (std::make_pair(x, y) == meta) break;
        for (int k = 0; k < 4; ++k) {
            const int nx = x + dx[k], ny = y + dy[k];
            if (nx < 0 || ny < 0 || nx >= ancho || ny >= alto) continue;
            if (t[ny][nx] == '*' || padre[ny][nx].first != -2) continue;
            padre[ny][nx] = {x, y};
            cola.push({nx, ny});
        }
    }
    if (padre[meta.second][meta.first].first == -2) return {};

    std::vector<std::pair<int, int>> camino;
    for (auto p = meta; p != inicio; p = padre[p.second][p.first]) camino.push_back(p);
    camino.push_back(inicio);
    return {camino.rbegin(), camino.rend()};
}

int main() {
    Tablero tablero = {
        "....................",
        ".R.....*............",
        "..*****.*.....***...",
        "..*.....*.....*.....",
        "..*.***.*.....*.....",
        "..*...*.*..****.....",
        "..*****.*...........",
        "........*.......X...",
        "....................",
        "....................",
    };

    Simulador simulador;
    simulador.setTablero(tablero);

    const auto camino = buscarCamino(tablero);
    if (camino.empty()) {
        std::cout << "No hay camino hasta la X\n";
        simulador.esperarCierre();
        return 0;
    }
    std::cout << "Camino de " << camino.size() - 1 << " pasos. Cierra la ventana para terminar antes.\n";

    simulador.pause(1000);
    for (std::size_t i = 1; i < camino.size() && simulador.abierto(); ++i) {
        const auto [ax, ay] = camino[i - 1];
        const auto [bx, by] = camino[i];
        tablero[ay][ax] = '-';  // casilla que dejamos: visitada
        tablero[by][bx] = 'R';  // casilla nueva: aquí está el robot
        simulador.setTablero(tablero);
        simulador.pause(250);  // 250 ms entre un paso y el siguiente
    }

    std::cout << "Fin del algoritmo. Cierra la ventana para salir.\n";
    simulador.esperarCierre();
    simulador.fin();
    return 0;
}
