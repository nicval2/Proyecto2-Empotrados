// audio.h
#ifndef AUDIO_H
#define AUDIO_H

#include "alt_types.h"

/* --- Tokens de Protocolo --- */
#define METADATA_MAGIC   0xDEADDA7A
#define AUDIO_EOS_TOKEN  0xFFFFFFFF  // Fin de canción normal
#define AUDIO_SKIP_TOKEN 0xFFFFFFFE  // Salto forzado (next/prev)

/* --- Comandos NIOS -> HPS --- */
#define CMD_NEXT  0x4E455854  // "NEXT"
#define CMD_PREV  0x50524556  // "PREV"

/* --- Variables Globales --- */
extern char artist_str[64];
extern char album_str[64];
extern char title_str[64];
extern int volume_shift;

/* --- Prototipos --- */

// Inicializa el hardware de audio
void audio_init(void);

// Bloquea hasta recibir el Magic Token del HPS
void audio_wait_handshake(void);

// Lee metadata (rate, channels, bits, strings)
// Retorna sample_rate
alt_u32 audio_receive_metadata(void);

// Revisa si hay datos en FIFO de entrada
int audio_fifo_has_data(void);

// Procesa una muestra de audio
// Retorna: 1 = OK, 0 = EOS normal, -1 = SKIP (next/prev)
int audio_process_sample(void);

// === NUEVAS FUNCIONES PARA COMUNICACIÓN NIOS -> HPS ===

// Envía comando al HPS (CMD_NEXT, CMD_PREV, etc.)
void audio_send_command(alt_u32 cmd);

// Descarta todos los datos en el FIFO hasta recibir EOS o SKIP
// Útil cuando cambiamos de canción
void audio_flush_fifo(void);

#endif
