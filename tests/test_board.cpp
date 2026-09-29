#include <stdexcept>
#include <string>
#include <vector>

#include "Board.h"
#include "check.h"

int main() {
    // Tablero válido a partir de un array plano.
    const char flat[] = "R.*"
                        "-.X";
    Board b = Board::fromFlat(flat, 3, 2, 3);
    CHECK(b.width() == 3 && b.height() == 2);
    CHECK(b.at(0, 0) == Cell::Robot && b.at(2, 0) == Cell::Obstacle);
    CHECK(b.at(0, 1) == Cell::Visited && b.at(2, 1) == Cell::Goal && b.at(1, 1) == Cell::Free);
    CHECK(b.hasRobot() && b.robotX() == 0 && b.robotY() == 0);

    // Con 'stride' mayor que el ancho se ignoran las columnas sobrantes.
    const char padded[] = "R.??" "..X?";
    Board p = Board::fromFlat(padded, 2, 2, 4);
    CHECK(p.width() == 2 && p.at(0, 1) == Cell::Free);

    // Otras fábricas dan el mismo resultado.
    Board s = Board::fromStrings({"R.*", "-.X"});
    CHECK(s == b);
    const char* rows[] = {"R.*", "-.X"};
    CHECK(Board::fromRowPointers(rows, 3, 2) == b);

    // Sin robot es válido.
    CHECK(!Board::fromStrings({"..", ".."}).hasRobot());

    // Validaciones.
    CHECK_THROWS(Board::fromFlat(nullptr, 3, 2, 3), std::invalid_argument);
    CHECK_THROWS(Board::fromFlat(flat, 0, 2, 3), std::invalid_argument);
    CHECK_THROWS(Board::fromFlat(flat, 3, -1, 3), std::invalid_argument);
    CHECK_THROWS(Board::fromFlat(flat, Board::kMaxSide + 1, 1, Board::kMaxSide + 1), std::invalid_argument);
    CHECK_THROWS(Board::fromFlat(flat, 4, 1, 3), std::invalid_argument);  // stride < ancho
    CHECK_THROWS(Board::fromStrings({"R.", "?."}), std::invalid_argument);  // carácter inválido
    CHECK_THROWS(Board::fromStrings({"R.", "R."}), std::invalid_argument);  // dos robots
    CHECK_THROWS(Board::fromStrings({"R..", ".."}), std::invalid_argument);  // filas desiguales
    CHECK_THROWS(Board::fromStrings({}), std::invalid_argument);
    CHECK_THROWS(Board::fromStrings({"r."}), std::invalid_argument);  // minúsculas no valen
    const char* holes[] = {"R.", nullptr};
    CHECK_THROWS(Board::fromRowPointers(holes, 2, 2), std::invalid_argument);

    // Tamaño máximo permitido.
    const std::vector<std::string> big(Board::kMaxSide, std::string(Board::kMaxSide, '.'));
    CHECK(Board::fromStrings(big).width() == Board::kMaxSide);
    return 0;
}
