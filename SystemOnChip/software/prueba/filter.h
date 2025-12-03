#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

/* Tipos de filtros/presets disponibles */
typedef enum {
    FILTER_NONE = 0,
    FILTER_BASS_BOOST,      // SW0: Realza graves
    FILTER_BASS_CUT,        // SW1: Reduce graves
    FILTER_TREBLE_BOOST,    // SW2: Realza agudos
    FILTER_TREBLE_CUT,      // SW3: Reduce agudos
    FILTER_VOCAL,           // SW4: Realza medios (voces)
    FILTER_ROCK,            // SW5: V-shape (graves+agudos)
    FILTER_POP,             // SW6: Medios+agudos
    FILTER_JAZZ,            // SW7: Graves suaves, agudos claros
    FILTER_LOWPASS,         // Filtro t�cnico
    FILTER_HIGHPASS         // Filtro t�cnico
} filter_type_t;

/* Inicializaci�n */
void filter_init(void);

/* Selecci�n de filtro */
void filter_set(filter_type_t type);

/* Obtener filtro actual */
filter_type_t filter_get(void);

/* Obtener nombre del filtro (para VGA) */
const char* filter_get_name(void);

/* Procesamiento de una muestra */
int16_t filter_process(int16_t sample);

/* Procesar switches y actualizar filtro autom�ticamente */
/* Retorna 1 si el filtro cambi� */
int filter_update_from_switches(int sw_value);

#endif
