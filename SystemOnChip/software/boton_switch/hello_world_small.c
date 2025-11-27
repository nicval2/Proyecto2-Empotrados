#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <unistd.h>
#include "filter.h"

/* --- TUS MÁSCARAS QUE YA FUNCIONAN --- */
#define BUTTON_PLAY_MASK  0x8  // Botón 1 (Bit 3)
#define BUTTON_NEXT_MASK  0x4  // Botón 2 (Bit 2)
#define BUTTON_PREV_MASK  0x2  // Botón 3 (Bit 1)

/* Variable global para depuración */
volatile int edge_capture;

/* --------------------------------------------------------------------
 * RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR) - (TU CÓDIGO)
 * -------------------------------------------------------------------- */
static void handle_button_interrupts(void* context)
{
    /* 1. Leer qué bit causó la interrupción */
    volatile int cast_edge_capture;
    cast_edge_capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);

    /* 2. Limpiar la interrupción */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, cast_edge_capture);

    /* 3. Lógica de los botones */
    if (cast_edge_capture & BUTTON_PLAY_MASK) {
        alt_putstr(">> Accion: PLAY (Boton 1)\n");
    }
    else if (cast_edge_capture & BUTTON_NEXT_MASK) {
        alt_putstr(">> Accion: NEXT (Boton 2)\n");
    }
    else if (cast_edge_capture & BUTTON_PREV_MASK) {
        alt_putstr(">> Accion: PREV (Boton 3)\n");
    }
}

/* --------------------------------------------------------------------
 * INICIALIZACIÓN DE BOTONES - (TU CÓDIGO)
 * -------------------------------------------------------------------- */
void init_buttons()
{
    // 1. Limpiar basura previa
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0x0);

    // 2. Registrar la ISR (PRIMERO esto)
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL,
        0x0
    );

    // 3. Habilitar interrupciones (DE ÚLTIMO esto)
    // Así evitas que salte una interrupción "huérfana" antes de tiempo.
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0xE);

    alt_putstr("Sistema de botones listo.\n");
}

/* --------------------------------------------------------------------
 * MAIN
 * -------------------------------------------------------------------- */
int main()
{
  alt_putstr("Iniciando sistema completo...\n");

  /* 1. Inicializar interrupciones de botones */
  init_buttons();

  /* 2. Variables para los Switches */
  int switch_val = 0;
  int prev_switch_val = -1; // -1 para forzar la impresión la primera vez

  /* Bucle infinito */
  while (1)
  {
      /* --- LEER SWITCHES (POLLING) --- */
      /* Leemos el valor actual del puerto de los switches */
      switch_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

      /* Solo imprimimos si el valor cambió respecto a la última vez */
      if (switch_val != prev_switch_val)
      {
    	  prev_switch_val = switch_val;

    	      alt_putstr("Filtro seleccionado: ");

    	      if (switch_val == 1)
    	      {
    	          filter_lowpass_enable();
    	          alt_putstr("Pasa-Bajas ACTIVADO\n");
    	      }
    	      else
    	      {
    	          filter_lowpass_disable();
    	          alt_putstr("Filtro DESACTIVADO\n");
    	      }
      }



      /* Pequeña pausa para estabilidad (opcional, ayuda a no saturar la consola) */
      usleep(100000); // 100ms
  }

  return 0;
}
