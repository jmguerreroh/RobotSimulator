#include "Overlay.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <vector>

namespace {

const char* const kVert = R"(#version 330 core
layout(location = 0) in vec2 aPos;    // en píxeles, origen arriba a la izquierda
layout(location = 1) in vec4 aColor;
uniform vec2 uScreen;
out vec4 vColor;
void main()
{
    vColor = aColor;
    gl_Position = vec4(aPos.x / uScreen.x * 2.0 - 1.0, 1.0 - aPos.y / uScreen.y * 2.0, 0.0, 1.0);
}
)";

const char* const kFrag = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() { FragColor = vColor; }
)";

const glm::vec4 kBannerColor{0.0f, 0.0f, 0.0f, 0.65f};
const glm::vec4 kTextColor{1.0f, 0.85f, 0.20f, 1.0f};

struct Vertex2D {
    glm::vec2 pos;
    glm::vec4 color;
};

// Fuente de 5x7: cada carácter son 7 filas de 5 columnas ('#' = píxel encendido).
const char* glyph(char c) {
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case 'A': return ".###.#...##...#######...##...##...#";
        case 'B': return "####.#...##...#####.#...##...#####.";
        case 'C': return ".###.#...##....#....#....#...#.###.";
        case 'D': return "####.#...##...##...##...##...#####.";
        case 'E': return "######....#....####.#....#....#####";
        case 'F': return "######....#....####.#....#....#....";
        case 'G': return ".###.#...##....#.####...##...#.###.";
        case 'H': return "#...##...##...#######...##...##...#";
        case 'I': return ".###...#....#....#....#....#...###.";
        case 'J': return "..###...#....#....#....#.#..#..##..";
        case 'K': return "#...##..#.#.#..##...#.#..#..#.#...#";
        case 'L': return "#....#....#....#....#....#....#####";
        case 'M': return "#...###.###.#.##.#.##...##...##...#";
        case 'N': return "#...###..##.#.##..###...##...##...#";
        case 'O': return ".###.#...##...##...##...##...#.###.";
        case 'P': return "####.#...##...#####.#....#....#....";
        case 'Q': return ".###.#...##...##...##.#.##..#..##.#";
        case 'R': return "####.#...##...#####.#.#..#..#.#...#";
        case 'S': return ".#####....#.....###.....#....#####.";
        case 'T': return "#####..#....#....#....#....#....#..";
        case 'U': return "#...##...##...##...##...##...#.###.";
        case 'V': return "#...##...##...##...##...#.#.#...#..";
        case 'W': return "#...##...##...##.#.##.#.##.#.#.#.#.";
        case 'X': return "#...##...#.#.#...#...#.#.#...##...#";
        case 'Y': return "#...##...#.#.#...#....#....#....#..";
        case 'Z': return "#####....#...#...#...#...#....#####";
        case '0': return ".###.#...##..###.#.###..##...#.###.";
        case '1': return "..#...##....#....#....#....#...###.";
        case '2': return ".###.#...#....#...#...#...#...#####";
        case '3': return ".###.#...#....#..##.....##...#.###.";
        case '4': return "...#...##..#.#.#..#.#####...#....#.";
        case '5': return "######....####.....#....##...#.###.";
        case '6': return ".###.#....#....####.#...##...#.###.";
        case '7': return "#####....#...#...#...#....#....#...";
        case '8': return ".###.#...##...#.###.#...##...#.###.";
        case '9': return ".###.#...##...#.####....#....#.###.";
        case '.': return "..............................#....";
        case ',': return "...........................#...#...";
        case ':': return "......#....#.........#....#........";
        case '-': return "...............###.................";
        case '!': return "..#....#....#....#....#.........#..";
        case '(': return "...#...#...#....#....#.....#.....#.";
        case ')': return ".#.....#.....#....#....#...#...#...";
        default: return nullptr;  // espacio o carácter sin dibujo
    }
}

void addRect(std::vector<Vertex2D>& v, float x, float y, float w, float h, const glm::vec4& color) {
    const glm::vec2 a(x, y), b(x + w, y), c(x + w, y + h), d(x, y + h);
    for (const glm::vec2& p : {a, b, c, a, c, d}) v.push_back({p, color});
}

}  // namespace

Overlay::Overlay() : shader_(kVert, kFrag) {
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<void*>(offsetof(Vertex2D, pos)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), reinterpret_cast<void*>(offsetof(Vertex2D, color)));
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

Overlay::~Overlay() {
    if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
    if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
}

void Overlay::drawBanner(const std::string& texto, const glm::ivec2& fb) {
    // Separa en líneas y mide la más larga.
    std::vector<std::string> lineas(1);
    for (char c : texto) {
        if (c == '\n') lineas.emplace_back();
        else lineas.back() += c;
    }
    std::size_t maxChars = 0;
    for (const std::string& l : lineas) maxChars = std::max(maxChars, l.size());
    if (maxChars == 0) return;

    // Tamaño del píxel de la fuente: proporcional a la ventana, sin pasarse de ancho.
    float px = std::max(2.0f, static_cast<float>(fb.y) / 220.0f);
    px = std::min(px, 0.9f * static_cast<float>(fb.x) / (6.0f * static_cast<float>(maxChars)));
    const float charW = 6.0f * px, lineH = 10.0f * px, pad = 2.0f * lineH / 3.0f;

    std::vector<Vertex2D> v;
    const float bannerH = static_cast<float>(lineas.size()) * lineH + 2.0f * pad - 3.0f * px;
    addRect(v, 0.0f, 0.0f, static_cast<float>(fb.x), bannerH, kBannerColor);
    for (std::size_t i = 0; i < lineas.size(); ++i) {
        const float x0 = 0.5f * (static_cast<float>(fb.x) - charW * static_cast<float>(lineas[i].size()) + px);
        const float y0 = pad + static_cast<float>(i) * lineH;
        for (std::size_t k = 0; k < lineas[i].size(); ++k) {
            const char* g = glyph(lineas[i][k]);
            if (g == nullptr) continue;
            for (int fila = 0; fila < 7; ++fila) {
                for (int col = 0; col < 5; ++col) {
                    if (g[fila * 5 + col] == '#') {
                        addRect(v, x0 + charW * static_cast<float>(k) + px * static_cast<float>(col),
                                y0 + px * static_cast<float>(fila), px, px, kTextColor);
                    }
                }
            }
        }
    }

    // Estado 2D: sin profundidad ni culling, con transparencia. Se restaura al terminar.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_.use();
    shader_.setVec2("uScreen", glm::vec2(fb));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(v.size() * sizeof(Vertex2D)), v.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}
