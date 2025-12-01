// buttons.h
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

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
