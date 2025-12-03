#include "buttons.h"
#include "audio.h"
#include "system.h"

/* ---- Variables globales ---- */
volatile int is_running = 1;
volatile int reset_request = 0;
volatile int skip_request = 0;
volatile int skip_direction = 0;

/* ---- Registros del PIO ---- */
#define PIO_DATA        (*(volatile uint32_t*)(REG_BUTTONS_BASE + 0x00))
#define PIO_DIR         (*(volatile uint32_t*)(REG_BUTTONS_BASE + 0x04))
#define PIO_IRQ_MASK    (*(volatile uint32_t*)(REG_BUTTONS_BASE + 0x08))
#define PIO_EDGE_CAP    (*(volatile uint32_t*)(REG_BUTTONS_BASE + 0x0C))

/*
 * Vector de interrupciones del runtime NIOS II
 * Este arreglo SIEMPRE existe aunque no uses HAL.
 */
extern void (*alt_irq[])(void);

/* ------------ ISR ------------ */
void buttons_isr(void)
{
    uint32_t edge = PIO_EDGE_CAP;
    PIO_EDGE_CAP = edge;   // Limpiar flags

    if (edge & BUTTON_PLAY_MASK) {
        is_running ^= 1;
    }
    else if (edge & BUTTON_NEXT_MASK) {
        audio_send_command(CMD_NEXT);
        skip_request = 1;
        skip_direction = 1;
    }
    else if (edge & BUTTON_PREV_MASK) {
        audio_send_command(CMD_PREV);
        skip_request = 1;
        skip_direction = -1;
    }
}

/* ------------ INIT ------------ */
void buttons_init(void)
{
    /* 1. Configurar como entrada */
    PIO_DIR = 0x00;

    /* 2. Limpiar edge capture */
    PIO_EDGE_CAP = 0x0F;

    /* 3. Habilitar interrupciones para KEY1–KEY3 */
    PIO_IRQ_MASK = 0x0E;  // bits 1, 2 y 3

    /* 4. Registrar ISR en el vector */
    alt_irq[REG_BUTTONS_IRQ] = buttons_isr;

    /* 5. Habilitar interrupción IRQ=1 en el CPU */
    uint32_t mask = (1 << REG_BUTTONS_IRQ);
    asm volatile("wrctl ienable, %0" :: "r"(mask));

    /* 6. Habilitar interrupciones globales */
    asm volatile("wrctl status, %0" :: "r"(1));
}

/* ------------ Limpieza ------------ */
void buttons_clear_skip(void)
{
    skip_request = 0;
    skip_direction = 0;
}
