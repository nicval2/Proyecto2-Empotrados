#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

/* Tipos de filtros disponibles */
typedef enum {
    FILTER_NONE = 0,
    FILTER_LOWPASS,
    FILTER_HIGHPASS
} filter_type_t;

/* Inicialización de filtros */
void filter_init(void);

/* Selección de filtro */
void filter_set(filter_type_t type);

/* Procesamiento de una muestra */
int16_t filter_process(int16_t x);

#endif
