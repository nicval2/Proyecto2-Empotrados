/*
 * test_peripherals_small.c
 *
 * Versión optimizada para "Small C Library".
 * Usa alt_putstr y alt_printf para ahorrar memoria.
 *
 */

#include "sys/alt_stdio.h"          // Librería ligera para impresión (no usar stdio.h)
#include "sys/alt_irq.h"            // Librería para interrupciones
#include "system.h"                 // Definiciones del hardware
#include "altera_avalon_pio_regs.h" // Macros para registros PIO
#include <unistd.h>                 // Para usleep (opcional)

// ================= VARIABLES GLOBALES (Volátiles) =================
volatile int edge_capture = 0;

// ================= RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR) =================
static void handle_button_interrupts(void* context)
{
    // 1. Leer y limpiar el registro de captura
    volatile int capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, capture);

    // 2. Guardar valor para el main
    edge_capture = capture;
}

// ================= INICIALIZACIÓN =================
void init_buttons()
{
    // Resetear capture register
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0xF);

    // Habilitar interrupciones (Mascara 0x7 = 3 botones)
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0x7);

    // Registrar ISR
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL,
        NULL
    );

    alt_putstr("Interrupciones iniciadas.\n");
}

// ================= MAIN =================
int main()
{
    int current_switch_val = 0;
    int prev_switch_val = -1;

    alt_putstr("Iniciando prueba ligera (alt_stdio)...\n");

    init_buttons();

    while (1)
    {
        // --- PARTE 1: BOTONES (Interrupción) ---
        if (edge_capture != 0)
        {
            if (edge_capture & 0x1) {
                alt_putstr("[IRQ] BTN 0: Play/Pause\n");
            }
            if (edge_capture & 0x2) {
                alt_putstr("[IRQ] BTN 1: Next\n");
            }
            if (edge_capture & 0x4) {
                alt_putstr("[IRQ] BTN 2: Prev\n");
            }
            edge_capture = 0;
        }

        // --- PARTE 2: SWITCHES (Polling) ---
        current_switch_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

        // Solo imprimimos si cambia el valor
        if (current_switch_val != prev_switch_val)
        {
            // Usamos alt_printf porque necesitamos imprimir una VARIABLE (%x)
            // alt_putstr no puede imprimir variables.
            alt_putstr("[SW] Nuevo valor (Hex): ");
            alt_printf("%x\n", current_switch_val);

            prev_switch_val = current_switch_val;
        }

        // Pequeño retardo para no saturar el CPU si no es necesario
        // (Aunque en bare-metal puro, un bucle vacío también sirve)
        for(volatile int i=0; i<10000; i++);
    }

    return 0;
}
