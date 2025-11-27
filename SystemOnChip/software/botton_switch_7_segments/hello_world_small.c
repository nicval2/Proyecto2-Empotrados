#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "altera_avalon_timer_regs.h"
#include "sys/alt_irq.h"
#include <unistd.h>

/* --- DEFINICIONES DE HARDWARE --- */
#define SEGMENTS7_BASE 0x3000
#define TIMER_BASE     0x3020

/* Máscaras de Botones */
#define BUTTON_PLAY_MASK  0x8  // Bit 3
#define BUTTON_NEXT_MASK  0x4  // Bit 2
#define BUTTON_PREV_MASK  0x2  // Bit 1

/* --- VARIABLES GLOBALES --- */
volatile int is_running = 1;     // 1: Run, 0: Pause
volatile int reset_request = 0;  // 1: Solicitar reset del reloj

/* --------------------------------------------------------------------
 * FUNCION: DISPLAY 7 SEGMENTOS
 * -------------------------------------------------------------------- */
void display_time_4seg(volatile int *hex_ptr, int minutes, int seconds)
{
    int seg7_table[10] = {
        0x40, 0x79, 0x24, 0x30, 0x19,
        0x12, 0x02, 0x78, 0x00, 0x10
    };

    int m1 = (minutes / 10) % 10;
    int m0 = minutes % 10;
    int s1 = (seconds / 10) % 10;
    int s0 = seconds % 10;

    int hex3 = seg7_table[m1];
    int hex2 = seg7_table[m0];
    int hex1 = seg7_table[s1];
    int hex0 = seg7_table[s0];

    *hex_ptr = (hex3 << 24) | (hex2 << 16) | (hex1 << 8) | hex0;
}

/* --------------------------------------------------------------------
 * INTERRUPCIÓN DE BOTONES (SEPARADA)
 * -------------------------------------------------------------------- */
static void handle_button_interrupts(void* context)
{
    // 1. Leer y limpiar la interrupción
    volatile int edge = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge);

    // --- Lógica de Botones Separada ---

    // CASO 1: PLAY / PAUSE
    if (edge & BUTTON_PLAY_MASK) {
        if (is_running == 1) {
            is_running = 0;
            alt_putstr(">> ACCION: PAUSE\n");
        } else {
            is_running = 1;
            alt_putstr(">> ACCION: PLAY\n");
        }
    }

    // CASO 2: NEXT TRACK
    else if (edge & BUTTON_NEXT_MASK) {
        // Por ahora resetea el reloj, pero aquí pondrás la lógica de "Siguiente Canción"
        reset_request = 1;
        is_running = 1;    // Si estaba en pausa, que arranque al cambiar de canción
        alt_putstr(">> ACCION: NEXT TRACK (Clock Reset)\n");
    }

    // CASO 3: PREV TRACK
    else if (edge & BUTTON_PREV_MASK) {
        // Por ahora resetea el reloj, pero aquí pondrás la lógica de "Canción Anterior"
        reset_request = 1;
        is_running = 1;
        alt_putstr(">> ACCION: PREV TRACK (Clock Reset)\n");
    }
}

/* --------------------------------------------------------------------
 * INICIALIZACIÓN DE BOTONES
 * -------------------------------------------------------------------- */
void init_buttons()
{
    // Limpiar basura
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0x0);

    // Registrar ISR (Primero)
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL, 0x0
    );

    // Habilitar máscara (Último)
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0xE);

    alt_putstr("Botones inicializados.\n");
}

/* --------------------------------------------------------------------
 * INICIALIZACIÓN DEL TIMER
 * -------------------------------------------------------------------- */
void init_timer_hardware() {
    // Configurar Timer: START | CONT | (STOP=0) | (ITO=0) -> 0x6
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x6);
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
    alt_putstr("Timer hardware iniciado.\n");
}

/* --------------------------------------------------------------------
 * MAIN
 * -------------------------------------------------------------------- */
int main()
{
    alt_putstr("--- INICIANDO REPRODUCTOR DE AUDIO SoC ---\n");

    init_buttons();
    init_timer_hardware();

    volatile int * segments7_ptr = (int *) SEGMENTS7_BASE;

    int elapsed_ms = 0;
    int elapsed_s = 0;
    int elapsed_m = 0;

    int switch_val = 0;
    int prev_switch_val = -1;

    // Estado inicial visual
    display_time_4seg(segments7_ptr, 0, 0);

    while (1)
    {
        /* 1. GESTIÓN DE RESET (Solicitado por NEXT o PREV) */
        if (reset_request) {
            elapsed_ms = 0;
            elapsed_s = 0;
            elapsed_m = 0;
            display_time_4seg(segments7_ptr, elapsed_m, elapsed_s);
            reset_request = 0; // Bajar bandera
        }

        /* 2. GESTIÓN DEL TIMER (Polling del bit TO) */
        int timer_status = IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE);

        // Si el bit de Timeout (bit 0) es 1, pasó un ciclo (ej. 1ms)
        if (timer_status & 0x1) {
            // Limpiar flag de hardware
            IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);

            // Solo avanzar tiempo si no está en pausa
            if (is_running) {
                elapsed_ms++;
                if (elapsed_ms >= 1000) {
                    elapsed_ms = 0;
                    elapsed_s++;

                    if (elapsed_s >= 60) {
                        elapsed_s = 0;
                        elapsed_m++;
                    }
                    display_time_4seg(segments7_ptr, elapsed_m, elapsed_s);
                }
            }
        }

        /* 3. GESTIÓN DE SWITCHES (Filtros) */
        switch_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);
        if (switch_val != prev_switch_val) {
            alt_putstr("Filtro Activo (Hex): ");
            alt_printf("%x\n", switch_val);
            prev_switch_val = switch_val;
        }
    }

    return 0;
}
