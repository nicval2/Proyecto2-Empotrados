#include "buttons.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <stddef.h>

/* Máscaras (Asegúrate que coincidan con tus conexiones físicas) */
#define BUTTON_PLAY_MASK  0x8  // KEY3
#define BUTTON_NEXT_MASK  0x4  // KEY2
#define BUTTON_PREV_MASK  0x2  // KEY1

/* Variables globales */
volatile int is_running = 1;
volatile int reset_request = 0;

/* ISR: Mantenla lo más corta posible */
void buttons_isr(void* context)
{
    // 1. Leer y limpiar el registro de captura de bordes
    volatile int edge = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge);

    // 2. Lógica rápida sin printf
    if (edge & BUTTON_PLAY_MASK)
    {
        is_running ^= 1;   // Toggle 1/0
    }
    else if (edge & BUTTON_NEXT_MASK)
    {
        reset_request = 1;
        is_running = 1;    // Asegurar que arranque al cambiar canción
    }
    else if (edge & BUTTON_PREV_MASK)
    {
        reset_request = 1;
        is_running = 1;
    }
}

void buttons_init(void)
{
    /* Limpiar flags previos */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0);

    /* Registrar ISR */
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        buttons_isr,
        NULL,
        0
    );

    /* Habilitar interrupciones para los 3 botones (Mask 0xE = 1110 binario) */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE,0x0E);
}
