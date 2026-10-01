// SessionLog.h - Registro de la sesión de simulación (fichero .log firmado).
//
// Guarda los datos del alumno (autor, email, práctica), el PRIMER tablero que se mostró (estado inicial) y el
// ÚLTIMO (estado final), y al terminar escribe un fichero de texto cuya última línea es el CHECKSUM
// de TODO el contenido anterior: un HMAC-SHA256 con clave. Sin la clave no se puede recalcular, de modo que
// cualquier edición posterior del fichero (autor, tableros, resumen...) invalida la firma.
//
// Formato (líneas terminadas en '\n', UTF-8):
//     === REGISTRO DE SIMULACION RobotSimulator ===
//     formato: 1 ... (cabecera) ... --- FIRMA ---
//     checksum: <64 hex>          <- HMAC-SHA256(clave, todos los bytes anteriores)
//
// AVISO: la clave va dentro del código de la biblioteca, así que esto detecta modificaciones del fichero, no es
// una defensa frente a quien lea el código fuente y reimplemente la firma (ver profesor/LEEME.md).
#pragma once

#include <chrono>
#include <string>

#include "Board.h"

class SessionLog {
public:
    // Validan: no vacío, <= 200 bytes, sin caracteres de control (evita "inyectar" líneas falsas en el log).
    // Lanzan std::invalid_argument.
    void setAutor(const std::string& autor);
    void setEmail(const std::string& email);  // además debe parecer un correo: algo@algo
    void setPractica(const std::string& practica);
    void setArchivo(const std::string& ruta);
    const std::string& archivo() const { return archivo_; }

    // Se llama con cada tablero válido que recibe el simulador.
    void record(const Board& board);
    bool hasBoard() const { return updates_ > 0; }

    // Contenido completo del fichero (incluida la línea de checksum final).
    std::string render(std::chrono::system_clock::time_point end, double durationSeconds) const;
    // Escribe el fichero (en binario, para que el checksum sea idéntico en todos los sistemas).
    // Lanza std::runtime_error si no se puede escribir. Devuelve la ruta escrita.
    std::string write() const;

    // HMAC-SHA256 (hex) de 'body' con la clave de la biblioteca.
    static std::string checksumOf(const std::string& body);

private:
    std::string autor_, email_, practica_;
    std::string archivo_ = "simulacion.log";
    Board initial_, last_;
    long updates_ = 0;
    std::chrono::system_clock::time_point startWall_;
    std::chrono::steady_clock::time_point startSteady_;
};
