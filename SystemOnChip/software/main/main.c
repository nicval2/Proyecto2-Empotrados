#include <stdio.h>
#include "system.h"
#include "7_segments.h"
#include "buttons.h"
#include "switches.h"
#include "timer.h"
#include "audio.h"   // <--- Agregar
#include "filter.h"  // <--- Agregar

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

int main()
{
    alt_putstr("--- INICIANDO SOC MULTITASKING ---\n");

    // Inicializaciones
    buttons_init();
    timer_init();
    filter_init();
    audio_init();    // <--- Inicializar Audio

    int ms = 0, s = 0, m = 0;

    display_time_4seg(SEG7_PTR, 0, 0);

    // Bucle infinito (Super Loop)
    while (1)
    {
        /* 1. AUDIO POLL
           Esta función intenta procesar UNA muestra de audio.
           Si no hay datos o el buffer está lleno, retorna rápido.
           Si is_running es 0 (pausa), no hace nada.
        */
        audio_poll();

        /* 2. GESTIÓN DEL RELOJ (TIMER) */
        if (timer_tick())
        {
            // El reloj solo avanza si el sistema está en "Play"
            if (is_running)
            {
                ms++;
                if (ms >= 1000)
                {
                    ms = 0;
                    s++;
                    if (s >= 60)
                    {
                        s = 0;
                        m++;
                    }
                    display_time_4seg(SEG7_PTR, m, s);
                }
            }
        }

        /* 3. RESET DE RELOJ (Solicitado por botones Next/Prev) */
        if (reset_request)
        {
            ms = 0; s = 0; m = 0;
            display_time_4seg(SEG7_PTR, m, s);
            reset_request = 0;
            // Aquí podrías agregar lógica para reiniciar el audio si fuera necesario
        }

        /* 4. SWITCHES (Control de Volumen y Filtros) */
        switches_poll();
    }

    return 0;
}
