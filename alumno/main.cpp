// main.cpp - TU PROGRAMA (plantilla creada por CMake la primera vez; a partir de ahora es tuyo).
#include <Simulador.h>

int main()
{
    Simulador simulador;

    // TUS DATOS: aparecen en el archivo de log que se genera al terminar (y que tienes que entregar).
    simulador.setAutor("Nombre Apellidos");
    simulador.setEmail("tu.correo@alumnos.urjc.es");
    simulador.setPractica("Nombre de la practica");

    // Tablero de 5 filas x 8 columnas.
    //   R robot   X objetivo   * obstaculo   . libre   - ya visitada
    char tablero[5][8] = {
        {'R','.','.','.','.','.','.','.'},
        {'.','.','*','*','*','.','.','.'},
        {'.','.','.','.','*','.','.','.'},
        {'.','.','.','.','.','.','X','.'},
        {'.','.','.','.','.','.','.','.'},
    };

    simulador.setTablero(tablero);   // abre la ventana; el programa sigue
    simulador.pause(1000);           // espera 1 s para ver el tablero

    // Ejemplo: mover el robot una casilla a la derecha dejando rastro.
    tablero[0][0] = '-';
    tablero[0][1] = 'R';
    simulador.setTablero(tablero);
    simulador.pause(1000);

    // TODO: tu algoritmo del robot.
    //   Cada vez que cambie el tablero, llama a simulador.setTablero(tablero)
    //   y a simulador.pause(ms) para que dé tiempo a verlo.

    simulador.esperarCierre();       // mantiene la ventana hasta que la cierres
    simulador.fin();                 // cierra y GUARDA el log (simulacion.log)
}
