// buttons.h
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/* Mascaras de botones */
#define BUTTON_PLAY_MASK  0x4 // Centro
#define BUTTON_NEXT_MASK  0x2 // Derecha
#define BUTTON_PREV_MASK  0x8 // Izquierda

/* Definicion manual de registros PIO */
#define BTN_PTR          ((volatile int *)REG_BUTTONS_BASE)
#define BTN_IRQ_MASK_IDX 2
#define BTN_EDGE_CAP_IDX 3

/* Variables globales */
extern volatile int is_running;
extern volatile int reset_request;
extern volatile int skip_request;
extern volatile int skip_direction;

/* Funciones */
void buttons_init(void);
void buttons_isr(void* context);
void buttons_clear_skip(void);

#endif
