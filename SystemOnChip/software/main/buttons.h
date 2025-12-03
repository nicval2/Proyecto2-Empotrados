#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/* Mascaras de botones */
#define BUTTON_PLAY_MASK  0x8  // KEY3 - Play/Pause
#define BUTTON_NEXT_MASK  0x4  // KEY2 - Siguiente
#define BUTTON_PREV_MASK  0x2  // KEY1 - Anterior

/* Variables globales */
extern volatile int is_running;
extern volatile int reset_request;
extern volatile int skip_request;
extern volatile int skip_direction;

/* Funciones */
void buttons_init(void);
void buttons_isr(void);
void buttons_clear_skip(void);

#endif
