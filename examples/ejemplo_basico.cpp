// ejemplo_basico.cpp - Lo mínimo para ver un tablero en 3D.
#include <Simulador.h>

int main() {
    Simulador simulador;

    // Y filas x X columnas. Se accede como tablero[fila][columna].
    char tablero[8][10] = {
        {'.', '.', '.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', 'R', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '*', '*', '*', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '*', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '*', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', 'X', '.', '.'},
        {'.', '.', '.', '.', '.', '.', '.', '.', '.', '.'},
    };

    simulador.setTablero(tablero);  // abre la ventana y sigue con el programa

    // Mover el robot dos casillas hacia la izquierda, dejando rastro.
    for (int paso = 0; paso < 2; ++paso) {
        simulador.pause(1000);  // espera 1 s para ver el tablero
        tablero[1][2 - paso] = '-';
        tablero[1][1 - paso] = 'R';
        simulador.setTablero(tablero);
    }

    simulador.esperarCierre();  // deja la ventana abierta hasta que la cierres
    simulador.fin();
    return 0;
}
