// buttons.c
#include "buttons.h"
#include "audio.h"  // Para audio_send_command()
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <stddef.h>

/* Máscaras de botones */
#define BUTTON_PLAY_MASK  0x8  // KEY3 - Play/Pause
#define BUTTON_NEXT_MASK  0x4  // KEY2 - Siguiente
#define BUTTON_PREV_MASK  0x2  // KEY1 - Anterior

/* Variables globales */
volatile int is_running = 1;
volatile int reset_request = 0;
volatile int skip_request = 0;      // Nueva: indica que hay que saltar canción
volatile int skip_direction = 0;    // Nueva: 1 = next, -1 = prev

/* ISR de botones */
void buttons_isr(void* context)
{
    // 1. Leer y limpiar registro de captura de bordes
    volatile int edge = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge);

    // 2. Procesar botones
    if (edge & BUTTON_PLAY_MASK)
    {
        // Toggle Play/Pause
        is_running ^= 1;
    }
    else if (edge & BUTTON_NEXT_MASK)
    {
        // Siguiente canción
        skip_request = 1;
        skip_direction = 1;
        is_running = 1;  // Asegurar reproducción

        // Enviar comando al HPS inmediatamente
        // Nota: Normalmente no es ideal hacer I/O en ISR,
        // pero es una escritura simple y rápida
        audio_send_command(CMD_NEXT);
    }
    else if (edge & BUTTON_PREV_MASK)
    {
        // Canción anterior
        skip_request = 1;
        skip_direction = -1;
        is_running = 1;

        audio_send_command(CMD_PREV);
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

    // Habilitar interrupciones para KEY1, KEY2, KEY3 (mask 0x0E = 0b1110)
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0x0E);
}

void buttons_clear_skip(void)
{
    skip_request = 0;
    skip_direction = 0;
}
