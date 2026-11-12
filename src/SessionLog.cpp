#include "SessionLog.h"

#include <ctime>
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>

#include "Sha256.h"

namespace {

constexpr const char* kLibraryVersion = "1.0.0";
constexpr const char* kUnset = "(sin indicar)";

// Disposición de los campos del registro.
std::string layoutTag() {
    static const std::uint8_t kLayout[] = {0x7a, 0xf7, 0x87, 0xb0, 0xfe, 0xab, 0xf4, 0x37, 0x47, 0x49, 0xd5,
                                           0xf2, 0x2d, 0xc0, 0x0c, 0x31, 0x2b, 0x4a, 0xd3, 0xe1, 0x6d, 0xf7,
                                           0x25, 0x66, 0x3c, 0xa9, 0x82, 0x8d, 0xc1, 0x04, 0xef, 0x9c};
    const Digest base = sha256("RobotSimulator/board-layout");
    std::string tag;
    for (std::size_t i = 0; i < sizeof kLayout; ++i) {
        tag.push_back(static_cast<char>(kLayout[i] ^ base[(i * 7 + 3) % base.size()]));
    }
    return tag;
}

void checkText(const std::string& value, const char* what) {
    if (value.empty()) throw std::invalid_argument(std::string("Simulador: ") + what + " no puede estar vacio");
    if (value.size() > 200)
        throw std::invalid_argument(std::string("Simulador: ") + what + " es demasiado largo (maximo 200 caracteres)");
    for (unsigned char c : value) {
        if (c < 0x20 || c == 0x7f)
            throw std::invalid_argument(std::string("Simulador: ") + what + " contiene caracteres de control (saltos de linea, tabuladores...)");
    }
}

std::string isoUtc(std::chrono::system_clock::time_point t) {
    const std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &tt);
#else
    gmtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

void appendBoard(std::ostringstream& os, const Board& b) {
    os << "dimensiones: " << b.width() << "x" << b.height() << "\n";
    for (int y = 0; y < b.height(); ++y) {
        for (int x = 0; x < b.width(); ++x) os << static_cast<char>(b.at(x, y));
        os << "\n";
    }
}

}  // namespace

void SessionLog::setAutor(const std::string& autor) {
    checkText(autor, "el autor");
    autor_ = autor;
}

void SessionLog::setEmail(const std::string& email) {
    checkText(email, "el email");
    const auto at = email.find('@');
    if (at == std::string::npos || at == 0 || at + 1 >= email.size() || email.find(' ') != std::string::npos ||
        email.find('@', at + 1) != std::string::npos)
        throw std::invalid_argument("Simulador: el email no es valido (formato esperado: nombre@dominio): " + email);
    email_ = email;
}

void SessionLog::setPractica(const std::string& practica) {
    checkText(practica, "el nombre de la practica");
    practica_ = practica;
}

void SessionLog::setArchivo(const std::string& ruta) {
    if (ruta.empty() || ruta.size() > 500)
        throw std::invalid_argument("Simulador: la ruta del archivo de log no es valida");
    archivo_ = ruta;
}

void SessionLog::record(const Board& board) {
    if (updates_ == 0) {
        initial_ = board;
        startWall_ = std::chrono::system_clock::now();
        startSteady_ = std::chrono::steady_clock::now();
    }
    last_ = board;
    ++updates_;
}

std::string SessionLog::checksumOf(const std::string& body) { return toHex(hmacSha256(layoutTag(), body)); }

std::string SessionLog::render(std::chrono::system_clock::time_point end, double durationSeconds) const {
    std::ostringstream os;
    os.imbue(std::locale::classic());
    os << "=== REGISTRO DE SIMULACION RobotSimulator ===\n";
    os << "formato: 1\n";
    os << "firma: HMAC-SHA256\n";
    os << "biblioteca: " << kLibraryVersion << "\n";
    os << "practica: " << (practica_.empty() ? kUnset : practica_) << "\n";
    os << "autor: " << (autor_.empty() ? kUnset : autor_) << "\n";
    os << "email: " << (email_.empty() ? kUnset : email_) << "\n";
    os << "inicio-utc: " << isoUtc(startWall_) << "\n";
    os << "fin-utc: " << isoUtc(end) << "\n";
    os << "duracion-s: " << std::fixed;
    os.precision(3);
    os << durationSeconds << "\n";
    os << "actualizaciones: " << updates_ << "\n";

    os << "\n--- ESTADO INICIAL ---\n";
    appendBoard(os, initial_);
    os << "\n--- ESTADO FINAL ---\n";
    appendBoard(os, last_);

    // Resumen derivado de los tableros (el verificador lo recalcula y lo contrasta).
    int visited = 0;
    for (int y = 0; y < last_.height(); ++y)
        for (int x = 0; x < last_.width(); ++x)
            if (last_.at(x, y) == Cell::Visited) ++visited;
    // Objetivos: las X del tablero inicial. Se cuentan como alcanzados los que en el tablero final están
    // pisados por el robot o marcados como visitados (el robot ya ha pasado por encima).
    int goals = 0, reached = 0;
    for (int y = 0; y < initial_.height(); ++y) {
        for (int x = 0; x < initial_.width(); ++x) {
            if (initial_.at(x, y) != Cell::Goal) continue;
            ++goals;
            if (x < last_.width() && y < last_.height() &&
                (last_.at(x, y) == Cell::Visited || last_.at(x, y) == Cell::Robot))
                ++reached;
        }
    }
    os << "\n--- RESUMEN ---\n";
    if (last_.hasRobot())
        os << "robot-final: columna " << last_.robotX() << ", fila " << last_.robotY() << "\n";
    else
        os << "robot-final: ninguno\n";
    os << "objetivo-alcanzado: " << reached << "/" << goals << "\n";
    os << "casillas-visitadas: " << visited << "\n";
    os << "\n--- FIRMA ---\n";

    std::string body = os.str();
    return body + "checksum: " + checksumOf(body) + "\n";
}

std::string SessionLog::write() const {
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - startSteady_).count();
    const std::string content = render(std::chrono::system_clock::now(), seconds);
    std::ofstream out(archivo_, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("no se pudo abrir el archivo de log para escribir: " + archivo_);
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.flush();
    if (!out) throw std::runtime_error("error escribiendo el archivo de log: " + archivo_);
    return archivo_;
}
