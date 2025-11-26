#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <unistd.h>

/* Asegúrate de que estas direcciones coincidan con tu system.h */
#ifndef REG_BUTTONS_BASE
    #define REG_BUTTONS_BASE 0x3010
    #define REG_BUTTONS_IRQ 1
    #define REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID 0
#endif

// Según tu imagen, los switches están en 0x3050
#ifndef REG_SWITCHES_BASE
    #define REG_SWITCHES_BASE 0x3050
#endif

#define BUTTON_PLAY_MASK  0x8
#define BUTTON_NEXT_MASK  0x4
#define BUTTON_PREV_MASK  0x2

volatile int estado_reproduccion = 0;
int switches_anteriores = -1; // Iniciamos en -1 para forzar la primera impresión

/* --- ISR DE BOTONES --- */
static void handle_button_interrupts(void* context)
{
    volatile int cast_edge_capture;
    cast_edge_capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, cast_edge_capture);

    if (cast_edge_capture & BUTTON_PLAY_MASK) {
        alt_putstr(">> BTN: Play/Pause\n");
    }
    else if (cast_edge_capture & BUTTON_NEXT_MASK) {
        alt_putstr(">> BTN: Next\n");
    }
    else if (cast_edge_capture & BUTTON_PREV_MASK) {
        alt_putstr(">> BTN: Prev\n");
    }
}

/* --- INICIALIZACIÓN --- */
void init_buttons()
{
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0x0);
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0xE);

    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL,
        NULL
    );
}

/* --- LÓGICA DE SWITCHES --- */
void revisar_switches()
{
    int sw_actual;

    // Leemos el puerto. Hacemos máscara con 0xFF por si acaso (8 bits)
    sw_actual = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE) & 0xFF;

    if (sw_actual != switches_anteriores)
    {
        // Imprimir estado solo si cambia
        alt_printf(">> SW (Evento): Valor cambio a HEX: 0x%x\n", sw_actual);

        // Lógica de ejemplo para filtros
        if (sw_actual & 0x01) alt_putstr("   [Filtro 1: ON]\n");
        else alt_putstr("   [Filtro 1: OFF]\n");

        if (sw_actual & 0x02) alt_putstr("   [Filtro 2: ON]\n");

        switches_anteriores = sw_actual;
    }
}

int main()
{
  alt_putstr("Iniciando Sistema de Audio...\n");
  init_buttons();
  alt_putstr("Botones listos. Iniciando bucle principal...\n");

  int debug_counter = 0;

  while (1)
  {
      revisar_switches();

      // Delay pequeño (100ms)
      usleep(100000);

      // -- DEBUG HEARTBEAT --
      // Esto imprimirá el estado CADA 2 SEGUNDOS aunque no muevas nada.
      // Si esto imprime "0x0" siempre aunque muevas los switches,
      // confirma que falta conectar los pines en el Top-Level Verilog.
      debug_counter++;
      if (debug_counter >= 20) {
          int raw_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE) & 0xFF;
          alt_printf("[DEBUG MONITOR] Leyendo Switches: 0x%x\n", raw_val);
          debug_counter = 0;
      }
  }

  return 0;
}
