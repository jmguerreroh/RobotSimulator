#include "Simulador.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "Board.h"
#include "RenderLoop.h"
#include "SessionLog.h"
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
    SessionLog log;            // protegido por 'mutex'
    bool logWritten = false;

    void publish(Board board) {
        std::lock_guard<std::mutex> lock(mutex);
        switch (state.load()) {
            case State::Closed:
                return;
            case State::Idle: {
                const Board first = board;  // para el registro (solo se anota si la ventana llega a abrirse)
                shared.publish(std::move(board));  // antes de arrancar: el primer frame ya lo dibuja
                try {
                    loop.start();
                } catch (...) {
                    state.store(State::Closed);
                    throw;
                }
                state.store(State::Running);
                log.record(first);
                return;
            }
            case State::Running:
                if (!loop.running()) {  // el usuario cerró la ventana
                    reap();
                    return;
                }
                log.record(board);
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
        writeLog();
    }

    // Escribe el .log una sola vez (la primera vez que se cierra el simulador) si llegó a mostrarse algún tablero.
    // Nunca lanza: fin() y el destructor deben ser seguros.
    void writeLog() {  // precondición: 'mutex' tomado
        if (logWritten || !log.hasBoard()) return;
        logWritten = true;
        try {
            const std::string path = log.write();
            std::cout << "[Simulador] Registro de la simulacion guardado en: " << path << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Simulador] ATENCION: no se pudo guardar el registro: " << e.what() << std::endl;
        }
    }

    template <class F>
    void withLog(F&& f) {
        std::lock_guard<std::mutex> lock(mutex);
        f(log);
    }

    bool isOpen() const {
        switch (state.load()) {
            case State::Idle: return true;
            case State::Running: return loop.running();
            case State::Closed: return false;
        }
        return false;
    }

    void pause(int ms) {
        if (ms < 0) throw std::invalid_argument("Simulador::pause: los milisegundos no pueden ser negativos");
        // Se espera a trozos para poder cortar la pausa en cuanto se cierre la ventana.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
        while (state.load() != State::Closed && isOpen()) {
            const auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::steady_clock::duration::zero()) return;
            std::this_thread::sleep_for(std::min<std::chrono::steady_clock::duration>(remaining, std::chrono::milliseconds(10)));
        }
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

void Simulador::setAutor(const std::string& autor) {
    impl_->withLog([&](SessionLog& l) { l.setAutor(autor); });
}

void Simulador::setEmail(const std::string& email) {
    impl_->withLog([&](SessionLog& l) { l.setEmail(email); });
}

void Simulador::setPractica(const std::string& practica) {
    impl_->withLog([&](SessionLog& l) { l.setPractica(practica); });
}

void Simulador::setDatos(const std::string& autor, const std::string& email, const std::string& practica) {
    // Se valida todo antes de guardar nada: o se aceptan los tres datos o ninguno.
    SessionLog probe;
    probe.setAutor(autor);
    probe.setEmail(email);
    probe.setPractica(practica);
    impl_->withLog([&](SessionLog& l) {
        l.setAutor(autor);
        l.setEmail(email);
        l.setPractica(practica);
    });
}

void Simulador::setArchivoLog(const std::string& ruta) {
    impl_->withLog([&](SessionLog& l) { l.setArchivo(ruta); });
}

void Simulador::fin() { impl_->fin(); }

bool Simulador::abierto() const { return impl_->isOpen(); }

void Simulador::pause(int ms) { impl_->pause(ms); }

void Simulador::esperarCierre() { impl_->waitForClose(); }
