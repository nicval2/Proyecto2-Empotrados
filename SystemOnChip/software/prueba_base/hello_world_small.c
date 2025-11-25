#include "sys/alt_stdio.h"
#include "sys/alt_irq.h"
#include "system.h"
#include "altera_avalon_pio_regs.h"

/* * Variable global volátil para que el bucle principal sepa
 * que ocurrió una interrupción.
 */
volatile int edge_capture;

/*
 * RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR)
 * Esta función se ejecuta automáticamente cuando presionas un botón via hardware.
 */
static void handle_button_interrupts(void* context)
{
    /* 1. Castear el contexto al registro base (opcional, pero buena práctica) */
    volatile int* edge_capture_ptr = (volatile int*) context;

    /* 2. Leer el registro de captura de bordes para saber qué botón fue */
    /* Store the value in the global variable */
    edge_capture = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);

    /* 3. ¡MUY IMPORTANTE! Limpiar la interrupción.
     * Si no escribes en este registro para limpiarlo, la CPU se quedará
     * atrapada ejecutando esta función infinitamente.
     */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, edge_capture);

    /* * Nota: No se recomienda usar "printf" o "alt_putstr" aquí dentro
     * porque son lentos y pueden bloquear otras interrupciones.
     * Es mejor cambiar una variable global y que el main imprima.
     * PERO, para este ejemplo simple, lo haremos aquí para que veas el efecto inmediato.
     */
    alt_putstr("¡Boton presionado! Interrupcion atendida.\n");
}

/*
 * FUNCIÓN DE INICIALIZACIÓN
 * Prepara el hardware del botón para generar interrupciones.
 */
void init_button_pio()
{
    /* 1. Habilitar la interrupción para los 4 botones (o los que tengas)
     * 0xf significa 1111 en binario, habilitando los bits 0, 1, 2 y 3.
     */
    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0xf);

    /* 2. Limpiar cualquier borde capturado previamente */
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0xf);

    /* 3. Registrar la interrupción en el HAL de Nios II
     * - ID del controlador
     * - Número de IRQ
     * - Nombre de tu función ISR
     * - Contexto (opcional)
     * - Flags (NULL)
     */
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        (void*)edge_capture_ptr,
        NULL
    );
}

int main()
{
  alt_putstr("Hola desde Nios II! Presiona un boton...\n");

  /* Inicializar las interrupciones */
  init_button_pio();

  /* Bucle infinito: La CPU puede hacer otras cosas aquí mientras espera */
  while (1) {
      /* Aquí podrías poner código que verifique 'edge_capture' si quisieras */
  }

  return 0;
}
