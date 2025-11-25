// include/audio_filter.h

#ifndef AUDIO_FILTER_H
#define AUDIO_FILTER_H

#include <stdint.h>
#include <limits.h>

// Definición para el tipo de datos de audio: 16 bits (común en audio digital)
typedef int16_t sample_t;

// Tamaño del buffer de transferencia (Ejemplo: 1024 samples)
#define BUFFER_SIZE 1024

typedef struct {
    sample_t samples[BUFFER_SIZE];
    uint32_t num_samples;
} audio_buffer_t;

// Tipos de filtro: Definidos por los switches de la tarjeta [cite: 40]
typedef enum {
    FILTRO_NINGUNO = 0,
    FILTRO_GRAVES = 1,
    FILTRO_MEDIOS = 2,
    FILTRO_AGUDOS = 3,
    FILTRO_CUSTOM_1 = 4
} filtro_t;

// Prototipo de la función de procesamiento DSP
void AUDIO_FILTRO_procesar_buffer(filtro_t filtro,
                                  const audio_buffer_t *input,
                                  audio_buffer_t *output);

#endif // AUDIO_FILTER_H
