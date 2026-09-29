#version 330 core
// basic.frag - Fragment shader: iluminación Blinn-Phong con una luz direccional.
//
//   ambiente  : luz base; más clara en las caras que miran hacia arriba (hemisférica).
//   difusa    : depende del ángulo entre la normal y la dirección de la luz.
//   especular : pequeño brillo (Blinn: vector intermedio entre luz y cámara).

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;

uniform vec3 uLightDir;  // dirección HACIA la luz (normalizada)
uniform vec3 uViewPos;   // posición de la cámara

out vec4 FragColor;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightDir);
    vec3 V = normalize(uViewPos - vWorldPos);
    vec3 H = normalize(L + V);

    vec3 ambient = mix(vec3(0.28), vec3(0.50), 0.5 + 0.5 * N.y);
    float diff = max(dot(N, L), 0.0);
    float spec = diff > 0.0 ? pow(max(dot(N, H), 0.0), 48.0) * 0.25 : 0.0;

    vec3 color = vColor.rgb * (ambient + 0.75 * diff) + vec3(spec);
    FragColor = vec4(color, vColor.a);
}
