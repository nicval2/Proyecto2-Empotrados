#include "switches.h"
#include "system.h"
#include "filter.h"
#include "ui.h"

static int last_sw = -1;

/* Inicializa estado de switches */
void switches_init(void)
{
    last_sw = -1;
}

/* Lee switches y aplica cambios de filtro */
void switches_update(void)
{
    int sw = *(volatile uint32_t *)(REG_SWITCHES_BASE);

    if (sw != last_sw) {

        /* Bits 0-7 controlan el ecualizador */
        if (filter_update_from_switches(sw & 0xFF)) {
            ui_update_filter();
        }

        last_sw = sw;
    }
}
