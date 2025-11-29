#include "switches.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "filter.h"
#include "audio.h" // Incluir audio para controlar volumen
#include <stdio.h>

static int last_switch = -1;

void switches_poll(void)
{
    int v = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

    if (v != last_switch)
    {
        // Lógica de Filtros (Bits 2 y 3)
        // 00 = Nada, 01 = LowPass, 10 = HighPass
        int filter_sel = (v >> 2) & 0x03;

        if (filter_sel == 0) {
            filter_set(FILTER_NONE);
            printf("Switch: Filtro OFF\n");
        } else if (filter_sel == 1) {
            filter_set(FILTER_LOWPASS);
            printf("Switch: Filtro LOW PASS\n");
        } else if (filter_sel == 2) {
            filter_set(FILTER_HIGHPASS);
            printf("Switch: Filtro HIGH PASS\n");
        }

        // Lógica de Volumen (Bits 0 y 1)
        // Usamos el valor directo como "shift".
        // 0 (00) = Max vol, 1 (01) = Shift 1 (mitad), 2 (10) = Shift 2 (cuarto)...
        int vol_sel = v & 0x03;
        audio_set_volume(vol_sel * 2); // Multiplico por 2 para que se note más (0, 2, 4, 6 shifts)
        printf("Switch: Volumen Shift %d\n", vol_sel * 2);

        last_switch = v;
    }
}
