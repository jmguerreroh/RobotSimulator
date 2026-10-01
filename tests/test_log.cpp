// Registro firmado: SHA-256/HMAC contra vectores oficiales, formato del log, validaciones y escritura.
// Con un argumento, escribe además el log de ejemplo en esa ruta (lo usa el test del verificador en Python).
#include <chrono>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "Board.h"
#include "SessionLog.h"
#include "Sha256.h"
#include "check.h"

static std::string leer(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    // ---- Vectores de prueba oficiales ---------------------------------------------------------
    CHECK(toHex(sha256("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(toHex(sha256("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(toHex(sha256("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");  // 2 bloques
    CHECK(toHex(sha256(std::string(1000000, 'a'))) ==
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    CHECK(toHex(hmacSha256("key", "The quick brown fox jumps over the lazy dog")) ==
          "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8");
    CHECK(toHex(hmacSha256(std::string(100, 'k'), "clave larga")).size() == 64);  // clave > 64 bytes

    // ---- Log de una "partida" ---------------------------------------------------------------------
    SessionLog log;
    CHECK(!log.hasBoard());
    log.setAutor("Ana Perez Lopez");
    log.setEmail("ana.perez@uni.es");
    log.setPractica("Practica 1: laberinto");
    log.record(Board::fromStrings({"R..", ".*.", "..X"}));
    log.record(Board::fromStrings({"-R.", ".*.", "..X"}));
    log.record(Board::fromStrings({"---", ".*-", "..R"}));  // llega a la X
    CHECK(log.hasBoard());

    const auto fin = std::chrono::system_clock::now();
    const std::string contenido = log.render(fin, 1.5);
    CHECK(contenido.rfind("=== REGISTRO DE SIMULACION RobotSimulator ===\n", 0) == 0);
    CHECK(contenido.find("autor: Ana Perez Lopez\n") != std::string::npos);
    CHECK(contenido.find("email: ana.perez@uni.es\n") != std::string::npos);
    CHECK(contenido.find("actualizaciones: 3\n") != std::string::npos);
    CHECK(contenido.find("--- ESTADO INICIAL ---\ndimensiones: 3x3\nR..\n.*.\n..X\n") != std::string::npos);
    CHECK(contenido.find("--- ESTADO FINAL ---\ndimensiones: 3x3\n---\n.*-\n..R\n") != std::string::npos);
    CHECK(contenido.find("objetivo-alcanzado: si\n") != std::string::npos);
    CHECK(contenido.find("casillas-visitadas: 4\n") != std::string::npos);
    CHECK(contenido.find("\r") == std::string::npos);  // solo '\n': el checksum es igual en todos los sistemas

    // La última línea es el checksum de TODO lo anterior.
    const std::size_t ultima = contenido.rfind("checksum: ");
    CHECK(ultima != std::string::npos && contenido.size() == ultima + 10 + 64 + 1 && contenido.back() == '\n');
    CHECK(contenido.substr(ultima + 10, 64) == SessionLog::checksumOf(contenido.substr(0, ultima)));

    // Cualquier cambio de un solo carácter cambia el checksum.
    std::string editado = contenido.substr(0, ultima);
    editado[editado.find("Ana")] = 'X';
    CHECK(SessionLog::checksumOf(editado) != contenido.substr(ultima + 10, 64));
    // Determinista.
    CHECK(log.render(fin, 1.5) == contenido);

    // ---- Validaciones: no se pueden "colar" líneas falsas en el log ------------------------------------
    SessionLog v;
    CHECK_THROWS(v.setAutor(""), std::invalid_argument);
    CHECK_THROWS(v.setAutor("Ana\nchecksum: 00"), std::invalid_argument);
    CHECK_THROWS(v.setAutor("Ana\tPerez"), std::invalid_argument);
    CHECK_THROWS(v.setAutor(std::string(201, 'a')), std::invalid_argument);
    CHECK_THROWS(v.setEmail("sin-arroba"), std::invalid_argument);
    CHECK_THROWS(v.setEmail("@dominio.es"), std::invalid_argument);
    CHECK_THROWS(v.setEmail("a b@dominio.es"), std::invalid_argument);
    CHECK_THROWS(v.setEmail("a@b@c.es"), std::invalid_argument);
    CHECK_THROWS(v.setPractica("linea1\r\nlinea2"), std::invalid_argument);
    CHECK_THROWS(v.setArchivo(""), std::invalid_argument);
    v.setAutor("José Muñoz");  // UTF-8 permitido

    // Sin datos: se marcan como "(sin indicar)".
    SessionLog sinDatos;
    sinDatos.record(Board::fromStrings({"R.X"}));
    CHECK(sinDatos.render(fin, 0.0).find("autor: (sin indicar)\n") != std::string::npos);

    // ---- Escritura a fichero (binario) y relectura ---------------------------------------------------
    const std::string ruta = argc > 1 ? argv[1] : "test_log_salida.log";
    log.setArchivo(ruta);
    CHECK(log.write() == ruta);
    const std::string leido = leer(ruta);
    const std::size_t u = leido.rfind("checksum: ");
    CHECK(leido.substr(u + 10, 64) == SessionLog::checksumOf(leido.substr(0, u)));
    SessionLog malo;
    malo.record(Board::fromStrings({"R.X"}));
    malo.setArchivo("/no/existe/carpeta/x.log");
    CHECK_THROWS(malo.write(), std::runtime_error);
    return 0;
}
