#include "../include/audio_filter.h"
#include <string.h> // Necesario para memcpy (copia de memoria)
#include <stdint.h> // Necesario para tipos de datos (aunque Eclipse lo marque como error, GCC lo encontrará)

// Definición del orden del filtro para el Promedio Móvil (ej: 4 puntos)
#define FILTER_ORDER 4

// Memoria de retardo (Delay Line) para almacenar las muestras anteriores del filtro
// Esta es la 'memoria de estado' necesaria para cualquier filtro IIR/FIR.
int16_t filter_delay_line[FILTER_ORDER] = {0};

void AUDIO_FILTRO_procesar_buffer(filtro_t filtro,
                                 const audio_buffer_t *input,
                                 audio_buffer_t *output)
{
    // 1. Verificación del tamaño
    // Asumimos que la comunicación es síncrona, por lo que los buffers deben ser iguales.
    if (input->num_samples != output->num_samples) {
        // En un sistema Bare Metal, esto podría ser manejado de forma más robusta
        return;
    }

    // 2. Estructura de Control de Filtros
    switch (filtro) {

        case FILTRO_NINGUNO:
            // Opción de bypass: simplemente copia la entrada a la salida.
            // Es la forma más eficiente de "desactivar" el filtro.
            memcpy(output->samples, input->samples, input->num_samples * sizeof(sample_t));
            break;

        case FILTRO_GRAVES:
            // Implementación simple de un filtro pasa-bajos (graves) usando un Promedio Móvil (FIR).

            for (uint32_t n = 0; n < input->num_samples; n++) {

                // A) Actualizar la línea de retardo (simula el Z^-1)
                for (int k = FILTER_ORDER - 1; k > 0; k--) {
                    filter_delay_line[k] = filter_delay_line[k - 1];
                }
                filter_delay_line[0] = input->samples[n]; // La muestra actual entra en el primer punto

                // B) Calcular la suma (Acumulación)
                int32_t accumulator = 0; // Usar int32_t (32 bits) para evitar desbordamiento al sumar 4 muestras de 16 bits.
                for (int k = 0; k < FILTER_ORDER; k++) {
                    accumulator += filter_delay_line[k];
                }

                // C) Calcular la salida (División por el orden del filtro)
                output->samples[n] = (sample_t)(accumulator / FILTER_ORDER);
            }
            break;

        case FILTRO_MEDIOS:
            // Tarea futura: Lógica para filtro de medios
            break;

        case FILTRO_AGUDOS:
            // Tarea futura: Lógica para filtro de agudos
            break;

        default:
            // En caso de un switch desconocido, haz bypass.
            memcpy(output->samples, input->samples, input->num_samples * sizeof(sample_t));
            break;
    }

    // 3. Finalización
    output->num_samples = input->num_samples;
}
