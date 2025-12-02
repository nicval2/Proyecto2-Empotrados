// buttons.c - Control de botones (v2.2)
#include "buttons.h"
#include "audio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <stddef.h>

/* Mascaras de botones */
#define BUTTON_PLAY_MASK  0x8  // KEY3 - Play/Pause
#define BUTTON_NEXT_MASK  0x4  // KEY2 - Siguiente
#define BUTTON_PREV_MASK  0x2  // KEY1 - Anterior

/* Variables globales */
volatile int is_running = 1;
volatile int reset_request = 0;
volatile int skip_request = 0;
volatile int skip_direction = 0;

/* ISR de botones */
void buttons_isr(void* context)
{
    // Leer y limpiar edge capture
    volatile int edge = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge);

    if (edge & BUTTON_PLAY_MASK)
    {
        // Toggle Play/Pause
        is_running ^= 1;
    }
    else if (edge & BUTTON_NEXT_MASK)
    {
        // Siguiente - enviar comando al HPS
        audio_send_command(CMD_NEXT);
        skip_request = 1;
        skip_direction = 1;
    }
    else if (edge & BUTTON_PREV_MASK)
    {
        // Anterior - enviar comando al HPS
        audio_send_command(CMD_PREV);
        skip_request = 1;
        skip_direction = -1;
    }
}

void buttons_init(void)
{
    // Limpiar flags previos
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0);

    // Registrar ISR
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        buttons_isr,
        NULL,
        0
    );

    // Habilitar interrupciones para KEY1, KEY2, KEY3
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0x0E);
}

void buttons_clear_skip(void)
{
    skip_request = 0;
    skip_direction = 0;
}
