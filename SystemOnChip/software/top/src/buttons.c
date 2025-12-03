#include "buttons.h"
#include "audio.h"
#include "system.h"
#include "sys/alt_irq.h"
#include <stddef.h>

volatile int is_running = 1;
volatile int reset_request = 0;
volatile int skip_request = 0;
volatile int skip_direction = 0;

void buttons_isr(void* context)
{
    // Leer Edge Capture directamente
    volatile int edge = BTN_PTR[BTN_EDGE_CAP_IDX];

    // Limpiar Edge Capture (escribiendo 1s en los bits que estaban en 1)
    BTN_PTR[BTN_EDGE_CAP_IDX] = edge;

    if (edge & BUTTON_PLAY_MASK)
    {
        is_running ^= 1;
    }
    else if (edge & BUTTON_NEXT_MASK)
    {
        audio_send_command(CMD_NEXT);
        skip_request = 1;
        skip_direction = 1;
    }
    else if (edge & BUTTON_PREV_MASK)
    {
        audio_send_command(CMD_PREV);
        skip_request = 1;
        skip_direction = -1;
    }
}

void buttons_init(void)
{
    // Limpiar edge capture previo
    BTN_PTR[BTN_EDGE_CAP_IDX] = 0xF;

    // Registrar ISR
    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        buttons_isr,
        NULL,
        0
    );

    // Habilitar interrupciones en el PIO (Mask Register)
    BTN_PTR[BTN_IRQ_MASK_IDX] = 0x0E; // 1110 binario (Key 1, 2, 3)
}

void buttons_clear_skip(void)
{
    skip_request = 0;
    skip_direction = 0;
}
