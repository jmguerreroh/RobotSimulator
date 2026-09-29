// Board.h - Tablero ya validado y convertido a un tipo fuertemente tipado.
//
// Un Board es un valor inmutable en la práctica: se construye validando la
// entrada del usuario, de modo que el resto del programa (render thread
// incluido) nunca ve caracteres inválidos ni accede fuera de rango.
#pragma once

#include <string>
#include <vector>

enum class Cell : char {
    Free = '.',
    Visited = '-',
    Obstacle = '*',
    Robot = 'R',
    Goal = 'X',
};

class Board {
public:
    static constexpr int kMaxSide = 256;  // lado máximo aceptado (256x256 = 65 536 casillas)

    Board() = default;  // tablero vacío 0x0

    // Las tres fábricas lanzan std::invalid_argument ante datos no válidos.
    static Board fromFlat(const char* data, int width, int height, int stride);
    static Board fromRowPointers(const char* const* rows, int width, int height);
    static Board fromStrings(const std::vector<std::string>& rows);

    int width() const { return width_; }
    int height() const { return height_; }
    bool empty() const { return cells_.empty(); }
    Cell at(int x, int y) const { return cells_[static_cast<std::size_t>(y) * width_ + x]; }

    bool hasRobot() const { return robotX_ >= 0; }
    int robotX() const { return robotX_; }
    int robotY() const { return robotY_; }

    bool operator==(const Board& o) const {
        return width_ == o.width_ && height_ == o.height_ && cells_ == o.cells_;
    }
    bool operator!=(const Board& o) const { return !(*this == o); }

private:
    template <class CharAt>
    static Board build(int width, int height, CharAt charAt);

    int width_ = 0;
    int height_ = 0;
    int robotX_ = -1;
    int robotY_ = -1;
    std::vector<Cell> cells_;
};
