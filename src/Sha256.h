// Sha256.h - SHA-256 y HMAC-SHA256 (FIPS 180-4 / RFC 2104) sin dependencias.
// Los usa el registro de la simulación para firmar el fichero .log.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

using Digest = std::array<std::uint8_t, 32>;

class Sha256 {
public:
    Sha256();
    void update(const void* data, std::size_t size);
    Digest finish();  // tras llamarlo el objeto no debe reutilizarse

private:
    void transform(const std::uint8_t* block);

    std::array<std::uint32_t, 8> state_;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t bufferSize_ = 0;
    std::uint64_t totalBytes_ = 0;
};

Digest sha256(const std::string& data);
Digest hmacSha256(const std::string& key, const std::string& message);
std::string toHex(const Digest& digest);  // 64 caracteres en minúscula
