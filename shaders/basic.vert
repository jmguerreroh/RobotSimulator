#version 330 core
// basic.vert - Vertex shader.
//
// Cadena de transformaciones:  clip = Projection * View * Model * posición
//   Model      (por instancia)  coloca/orienta/escala el objeto en el mundo.
//   View       (uniform)        mueve el mundo para verlo desde la cámara.
//   Projection (uniform)        aplica la perspectiva.
// Model llega como atributo de instancia (locations 2..5): así todas las casillas
// de un mismo tipo se dibujan con UNA llamada, cada una con su propia matriz.

layout(location = 0) in vec3 aPosition;  // posición del vértice (espacio del objeto)
layout(location = 1) in vec3 aNormal;    // normal del vértice
layout(location = 2) in mat4 aModel;     // ocupa las locations 2, 3, 4 y 5
layout(location = 6) in vec4 aColor;     // color de la instancia

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;

void main()
{
    vec4 world = aModel * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    // La matriz normal (inversa traspuesta) mantiene las normales correctas con escalados no uniformes.
    vNormal = transpose(inverse(mat3(aModel))) * aNormal;
    vColor = aColor;
    gl_Position = uProjection * uView * world;
}
