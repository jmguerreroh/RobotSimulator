// Prueba el comportamiento de la API que NO necesita abrir una ventana
// (funciona en máquinas sin pantalla, como los servidores de CI).
#include <Simulador.h>

#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

#include "check.h"

int main() {
    Simulador sim;
    CHECK(sim.abierto());  // aún no se abrió nada, pero tampoco se ha cerrado

    // Los errores se detectan ANTES de abrir ninguna ventana.
    const char ok[] = "R.X.";
    CHECK_THROWS(sim.setTablero(static_cast<const char*>(nullptr), 2, 2), std::invalid_argument);
    CHECK_THROWS(sim.setTablero(ok, 0, 2), std::invalid_argument);
    CHECK_THROWS(sim.setTablero(ok, 2, 0), std::invalid_argument);
    CHECK_THROWS(sim.setTablero("R?X.", 2, 2), std::invalid_argument);
    CHECK_THROWS(sim.setTablero(std::vector<std::string>{"R.", "R."}), std::invalid_argument);
    CHECK_THROWS(sim.setTablero(std::vector<std::string>{}), std::invalid_argument);
    char malo[2][2] = {{'R', 'q'}, {'.', '.'}};
    CHECK_THROWS(sim.setTablero(malo), std::invalid_argument);
    CHECK_THROWS(sim.setTablero(malo, 3, 2), std::invalid_argument);  // tamX > columnas del array
    CHECK(sim.abierto());

    // pause(): espera lo indicado; con ventana cerrada vuelve de inmediato; ms negativos son un error.
    {
        using Reloj = std::chrono::steady_clock;
        auto t0 = Reloj::now();
        sim.pause(0);
        sim.pause(120);  // aún no hay ventana: espera completa
        const auto ms = std::chrono::duration<double, std::milli>(Reloj::now() - t0).count();
        CHECK(ms >= 115.0 && ms < 400.0);
        CHECK_THROWS(sim.pause(-1), std::invalid_argument);
    }

    // fin() es idempotente, y tras fin() setTablero valida pero no hace nada.
    sim.fin();
    sim.fin();
    CHECK(!sim.abierto());
    sim.esperarCierre();  // vuelve de inmediato
    char bueno[2][3] = {{'R', '.', 'X'}, {'.', '*', '.'}};
    sim.setTablero(bueno);
    sim.setTablero(&bueno[0][0], 3, 2);
    sim.setTablero(bueno, 3, 2);
    sim.setTablero(std::vector<std::string>{"R.X"});
    CHECK_THROWS(sim.setTablero(malo), std::invalid_argument);
    CHECK(!sim.abierto());
    {
        const auto t0 = std::chrono::steady_clock::now();
        sim.pause(5000);  // cerrado: no debe esperar
        CHECK(std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(200));
    }
    return 0;
}
