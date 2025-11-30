#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/* Variables globales expuestas al main */
extern volatile int is_running;
extern volatile int reset_request;

void buttons_init(void);
// La ISR no necesita ser pública, pero no hace daño dejarla aquí
void buttons_isr(void* context);

#endif
