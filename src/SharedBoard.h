// SharedBoard.h - Único punto de intercambio de datos entre el hilo del alumno
// y el hilo de render.
//
// Diseño: un mutex protege una copia del último tablero y un contador de
// versión. El alumno publica (sustituye la copia); el render "pregunta si hay
// algo más nuevo" y, si lo hay, se lleva su PROPIA copia. Así el render nunca
// lee memoria compartida fuera del mutex, y si el alumno publica 500 veces por
// segundo, el render simplemente toma la última (estado siempre coherente).
// La sección crítica es diminuta (un swap / una copia de <= 64 KiB).
#pragma once

#include <cstdint>
#include <mutex>
#include <utility>

#include "Board.h"

class SharedBoard {
public:
    void publish(Board board) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::swap(board_, board);
            ++version_;
        }
        // 'board' contiene ahora el tablero antiguo y se libera fuera del mutex.
    }

    // Si hay una versión posterior a 'seenVersion', la copia en 'out' y devuelve true.
    bool fetchIfNewer(std::uint64_t& seenVersion, Board& out) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (version_ == seenVersion) return false;
        out = board_;
        seenVersion = version_;
        return true;
    }

private:
    mutable std::mutex mutex_;
    Board board_;
    std::uint64_t version_ = 0;
};
