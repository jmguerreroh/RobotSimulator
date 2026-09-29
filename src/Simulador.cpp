#include "Simulador.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include "Board.h"
#include "RenderLoop.h"
#include "SharedBoard.h"

// Estados del simulador (ver docs/DEVELOPER.md):
//
//   Idle --setTablero--> Running --fin()/ventana cerrada--> Closed
//     \------------------fin()---------------------------------^
//
// Closed es terminal: setTablero() sigue validando el tablero (para que los
// errores del alumno se vean siempre) pero no hace nada más.
struct Simulador::Impl {
    enum class State { Idle, Running, Closed };

    SharedBoard shared;
    RenderLoop loop{shared};
    std::mutex mutex;  // serializa publish / fin / reap; el hilo de render NUNCA lo toma
    std::atomic<State> state{State::Idle};

    void publish(Board board) {
        std::lock_guard<std::mutex> lock(mutex);
        switch (state.load()) {
            case State::Closed:
                return;
            case State::Idle:
                shared.publish(std::move(board));  // antes de arrancar: el primer frame ya lo dibuja
                try {
                    loop.start();
                } catch (...) {
                    state.store(State::Closed);
                    throw;
                }
                state.store(State::Running);
                return;
            case State::Running:
                if (!loop.running()) {  // el usuario cerró la ventana
                    reap();
                    return;
                }
                shared.publish(std::move(board));
                return;
        }
    }

    void fin() {
        std::lock_guard<std::mutex> lock(mutex);
        if (state.load() == State::Running) {
            loop.requestStop();
            reap();
        }
        state.store(State::Closed);
    }

    bool isOpen() const {
        switch (state.load()) {
            case State::Idle: return true;
            case State::Running: return loop.running();
            case State::Closed: return false;
        }
        return false;
    }

    void waitForClose() {
        if (state.load() != State::Running) return;
        while (loop.running()) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        std::lock_guard<std::mutex> lock(mutex);
        if (state.load() == State::Running) reap();
    }

private:
    void reap() {  // precondición: 'mutex' tomado
        loop.join();
        state.store(State::Closed);
    }
};

Simulador::Simulador() : impl_(std::make_unique<Impl>()) {}

Simulador::~Simulador() {
    try {
        impl_->fin();
    } catch (...) {
        // Un destructor no debe lanzar excepciones.
    }
}

void Simulador::setTablero(const char* tablero, int tamX, int tamY) {
    impl_->publish(Board::fromFlat(tablero, tamX, tamY, tamX));
}

void Simulador::setTablero(const char* const* filas, int tamX, int tamY) {
    impl_->publish(Board::fromRowPointers(filas, tamX, tamY));
}

void Simulador::setTablero(const std::vector<std::string>& filas) { impl_->publish(Board::fromStrings(filas)); }

void Simulador::setTableroConPaso(const char* tablero, int tamX, int tamY, int paso) {
    impl_->publish(Board::fromFlat(tablero, tamX, tamY, paso));
}

void Simulador::fin() { impl_->fin(); }

bool Simulador::abierto() const { return impl_->isOpen(); }

void Simulador::esperarCierre() { impl_->waitForClose(); }
