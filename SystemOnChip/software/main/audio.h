#ifndef AUDIO_DRIVER_H
#define AUDIO_DRIVER_H

#include <alt_types.h>

/* Constante para verificar sincronización */
#define METADATA_MAGIC 0xDEADDA7A

/* Inicializa el chip Wolfson (I2C) y los punteros */
void audio_init(void);

/* Retorna 1 si hay datos esperando en la FIFO del ARM, 0 si está vacía */
int audio_fifo_has_data(void);

/* Lee un dato de 32 bits de la FIFO del ARM (Bloqueante) */
alt_u32 audio_read_fifo(void);

/* Envía una muestra de audio al Codec (espera espacio autom.) */
void audio_play_sample(alt_u32 sample);

#endif
