#include "Board.h"

#include <stdexcept>

namespace {

void validateSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Simulador: el tamano del tablero debe ser positivo (recibido " +
                                    std::to_string(width) + "x" + std::to_string(height) + ")");
    }
    if (width > Board::kMaxSide || height > Board::kMaxSide) {
        throw std::invalid_argument("Simulador: tablero demasiado grande (" + std::to_string(width) +
                                    "x" + std::to_string(height) + "); el maximo es " +
                                    std::to_string(Board::kMaxSide) + "x" +
                                    std::to_string(Board::kMaxSide));
    }
}

Cell parseCell(char c, int x, int y) {
    switch (c) {
        case '.': return Cell::Free;
        case '-': return Cell::Visited;
        case '*': return Cell::Obstacle;
        case 'R': return Cell::Robot;
        case 'X': return Cell::Goal;
        default: break;
    }
    throw std::invalid_argument("Simulador: caracter no valido '" + std::string(1, c) +
                                "' (codigo " + std::to_string(static_cast<int>(static_cast<unsigned char>(c))) +
                                ") en fila " + std::to_string(y) + ", columna " + std::to_string(x) +
                                ". Solo se admiten: . - * R X");
}

}  // namespace

template <class CharAt>
Board Board::build(int width, int height, CharAt charAt) {
    validateSize(width, height);
    Board board;
    board.width_ = width;
    board.height_ = height;
    board.cells_.reserve(static_cast<std::size_t>(width) * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell cell = parseCell(charAt(x, y), x, y);
            if (cell == Cell::Robot) {
                if (board.robotX_ >= 0) {
                    throw std::invalid_argument("Simulador: hay mas de un robot 'R' en el tablero (fila " +
                                                std::to_string(y) + ", columna " + std::to_string(x) + ")");
                }
                board.robotX_ = x;
                board.robotY_ = y;
            }
            board.cells_.push_back(cell);
        }
    }
    return board;
}

Board Board::fromFlat(const char* data, int width, int height, int stride) {
    if (data == nullptr) throw std::invalid_argument("Simulador: el puntero al tablero es nulo");
    validateSize(width, height);
    if (stride < width) {
        throw std::invalid_argument("Simulador: tamX (" + std::to_string(width) +
                                    ") es mayor que las columnas del array (" + std::to_string(stride) + ")");
    }
    return build(width, height, [&](int x, int y) {
        return data[static_cast<std::size_t>(y) * stride + x];
    });
}

Board Board::fromRowPointers(const char* const* rows, int width, int height) {
    if (rows == nullptr) throw std::invalid_argument("Simulador: el puntero al tablero es nulo");
    validateSize(width, height);
    for (int y = 0; y < height; ++y) {
        if (rows[y] == nullptr) {
            throw std::invalid_argument("Simulador: la fila " + std::to_string(y) + " es un puntero nulo");
        }
    }
    return build(width, height, [&](int x, int y) { return rows[y][x]; });
}

Board Board::fromStrings(const std::vector<std::string>& rows) {
    if (rows.empty()) throw std::invalid_argument("Simulador: el tablero no tiene filas");
    const std::size_t width = rows.front().size();
    for (std::size_t y = 0; y < rows.size(); ++y) {
        if (rows[y].size() != width) {
            throw std::invalid_argument("Simulador: la fila " + std::to_string(y) +
                                        " no mide lo mismo que la primera (" + std::to_string(rows[y].size()) +
                                        " frente a " + std::to_string(width) + ")");
        }
    }
    validateSize(static_cast<int>(width), static_cast<int>(rows.size()));
    return build(static_cast<int>(width), static_cast<int>(rows.size()),
                 [&](int x, int y) { return rows[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]; });
}
