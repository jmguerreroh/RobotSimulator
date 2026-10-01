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

// Clave de firma (enmascarada: solo evita que aparezca tal cual al buscar con grep; NO es un secreto real).
// Debe coincidir con CLAVE en profesor/verificar_log.py.
std::string logKey() {
    static const unsigned char masked[] = {0x28, 0x12, 0x45, 0x03, 0x19, 0x1a, 0xbe, 0xb1, 0xa5, 0xd1, 0xd1,
                                           0x95, 0x8f, 0xc3, 0xe6, 0xfa, 0xe9, 0xba, 0x8c, 0xeb, 0x8b, 0xba,
                                           0xcc, 0xdf, 0x7a, 0x45, 0x21, 0x74, 0x5c, 0x13, 0x42, 0x63};
    std::string key;
    for (std::size_t i = 0; i < sizeof masked; ++i) {
        key.push_back(static_cast<char>(masked[i] ^ static_cast<unsigned char>((0x5A + i * 7) & 0xFF)));
    }
    return key;
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

std::string SessionLog::checksumOf(const std::string& body) { return toHex(hmacSha256(logKey(), body)); }

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
    bool anyGoal = false, reached = false;
    for (int y = 0; y < initial_.height(); ++y) {
        for (int x = 0; x < initial_.width(); ++x) {
            if (initial_.at(x, y) != Cell::Goal) continue;
            anyGoal = true;
            if (last_.hasRobot() && last_.robotX() == x && last_.robotY() == y) reached = true;
        }
    }
    os << "\n--- RESUMEN ---\n";
    if (last_.hasRobot())
        os << "robot-final: columna " << last_.robotX() << ", fila " << last_.robotY() << "\n";
    else
        os << "robot-final: ninguno\n";
    os << "objetivo-alcanzado: " << (!anyGoal ? "sin-objetivo" : (reached ? "si" : "no")) << "\n";
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
