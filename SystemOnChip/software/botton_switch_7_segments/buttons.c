#include "buttons.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"
#include <stdio.h>

/* Máscaras */
#define BUTTON_PLAY_MASK  0x8
#define BUTTON_NEXT_MASK  0x4
#define BUTTON_PREV_MASK  0x2

/* Variables globales */
volatile int is_running = 1;
volatile int reset_request = 0;

/* ISR real */
void buttons_isr(void* context)
{
    volatile int edge = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge);

    if (edge & BUTTON_PLAY_MASK)
    {
        is_running ^= 1;   // Toggle
        alt_putstr(">> PLAY/PAUSE (%d)\n", is_running);
    }
    else if (edge & BUTTON_NEXT_MASK)
    {
        reset_request = 1;
        is_running = 1;
        alt_putstr(">> NEXT Track (Reset Clock)\n");
    }
    else if (edge & BUTTON_PREV_MASK)
    {
        reset_request = 1;
        is_running = 1;
        alt_putstr(">> PREV Track (Reset Clock)\n");
    }
}

void buttons_init(void)
{
    /* limpiar flags */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0);

    /* registrar ISR */
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        buttons_isr,
        NULL,
        0
    );

    /* habilitar interrupciones */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0x0E);
}
