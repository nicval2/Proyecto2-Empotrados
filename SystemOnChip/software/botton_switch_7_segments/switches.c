#include "switches.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include <stdio.h>

static int last_switch = -1;

void switches_poll(void)
{
    int v = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

    if (v != last_switch)
    {
    	alt_putstr("Filtro activo (Hex): %x\n", v);
        last_switch = v;
    }
}
