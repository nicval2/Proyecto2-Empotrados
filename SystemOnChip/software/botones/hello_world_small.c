#include "sys/alt_stdio.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <unistd.h>

/* --- CORRECCIÓN DE MÁSCARAS SEGÚN TUS PRUEBAS --- */
/* Botón 1 (Bit 3) */
#define BUTTON_PLAY_MASK  0x8
/* Botón 2 (Bit 2) */
#define BUTTON_NEXT_MASK  0x4
/* Botón 3 (Bit 1) */
#define BUTTON_PREV_MASK  0x2

/* Variable global para depuración (opcional) */
volatile int edge_capture;

/* --------------------------------------------------------------------
 * RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR)
 * -------------------------------------------------------------------- */
static void handle_button_interrupts(void* context)
{
    /* 1. Leer qué bit causó la interrupción (Edge Capture) */
    volatile int cast_edge_capture;
    cast_edge_capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);

    /* 2. Limpiar la interrupción escribiendo en el mismo registro */
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
 * INICIALIZACIÓN
 * -------------------------------------------------------------------- */
void init_buttons()
{
    /* Limpiar cualquier basura previa */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0x0);

    /* * Habilitar interrupciones para los bits 3, 2 y 1.
     * Binario: 1110 = Hexadecimal: 0xE
     */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0xE);

    /* Registrar la ISR */
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL,
        0x0
    );

    alt_putstr("Sistema de botones listo.\n");
}

int main()
{ 
  alt_putstr("Iniciando programa con Interrupciones...\n");

  init_buttons();

  while (1)
  {
      /* El procesador está libre para hacer otras cosas aquí */
  }

  return 0;
}
