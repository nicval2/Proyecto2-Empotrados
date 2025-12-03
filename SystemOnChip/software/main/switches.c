#include "switches.h"
#include "system.h"
#include "filter.h"
#include "ui.h"     // para ui_update_filter()

static int last_sw = -1;

void switches_init(void)
{
    last_sw = -1;
}

void switches_update(void)
{
    int sw = *(volatile uint32_t *)(REG_SWITCHES_BASE);

    if (sw != last_sw) {
        /* Bits 0–7 controlan el ecualizador */
        if (filter_update_from_switches(sw & 0xFF)) {
            ui_update_filter();
        }

        last_sw = sw;
    }
}
