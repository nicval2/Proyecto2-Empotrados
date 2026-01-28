#include "player.h"

int main()
{
    /* 1. Inicializar todos los subsistemas */
    player_init();

    /* 2. Ejecutar el bucle principal del reproductor */
    player_loop();

    return 0;
}
