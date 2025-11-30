#ifndef AUDIO_H
#define AUDIO_H
#define AUDIO_EOS_TOKEN 0xFFFFFFFF  // Definimos el token aqui tambien
#include "alt_types.h"

/* --- Variables Globales (para leer desde main) --- */
extern char artist_str[64];
extern char album_str[64];
extern char title_str[64];

/* Variable de control de volumen (modificable desde main/switches) */
extern int volume_shift;

/* --- Prototipos de Funciones --- */

// Inicializa el hardware de audio y limpia FIFOs
void audio_init(void);

// Bloquea la ejecución hasta recibir el "Magic Token" del HPS
void audio_wait_handshake(void);

// Lee la configuración (Rate, Channels, Bits) y los Strings de metadata
// Devuelve el sample_rate para que el main pueda contar el tiempo
alt_u32 audio_receive_metadata(void);

// Revisa si hay datos en la FIFO de entrada (HPS -> FPGA)
// Retorna 1 si hay datos, 0 si está vacía
int audio_fifo_has_data(void);

// Lee un dato de la FIFO, aplica Filtro, aplica Volumen y lo envía al Audio Core
// Retorna el dato crudo procesado (útil si quisieras visualizarlo)
int audio_process_sample(void);


#endif
