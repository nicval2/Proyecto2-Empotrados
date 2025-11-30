#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/* Variables globales expuestas al main */
extern volatile int is_running;
extern volatile int reset_request;

void buttons_init(void);
// La ISR no necesita ser p�blica, pero no hace da�o dejarla aqu�
void buttons_isr(void* context);

#endif
