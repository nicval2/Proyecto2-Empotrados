// buttons.h
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>

/* Variables globales expuestas al main */
extern volatile int is_running;      // 1 = reproduciendo, 0 = pausado
extern volatile int reset_request;   // Legacy (puede usarse para reset de tiempo)
extern volatile int skip_request;    // 1 = se pidió saltar canción
extern volatile int skip_direction;  // 1 = next, -1 = prev

/* Funciones */
void buttons_init(void);
void buttons_isr(void* context);
void buttons_clear_skip(void);  // Limpia flags de skip después de procesarlos

#endif
