// audio.h - Con handshake de inicialización
#ifndef AUDIO_H
#define AUDIO_H

#include "alt_types.h"

/* --- Tokens de Protocolo --- */
#define METADATA_MAGIC   0xDEADDA7A
#define AUDIO_EOS_TOKEN  0xFFFFFFFF
#define AUDIO_SKIP_TOKEN 0xFFFFFFFE

/* --- Comandos NIOS -> HPS --- */
#define CMD_NEXT   0x4E455854  /* "NEXT" */
#define CMD_PREV   0x50524556  /* "PREV" */
#define CMD_READY  0x52454459  /* "REDY" - Señal de inicialización */

/* --- Variables Globales --- */
extern char artist_str[64];
extern char album_str[64];
extern char title_str[64];
extern int volume_shift;

/* --- Prototipos --- */
void audio_init(void);
void audio_wait_handshake(void);
alt_u32 audio_receive_metadata(void);
int audio_fifo_has_data(void);
int audio_process_sample(void);
void audio_send_command(alt_u32 cmd);

#endif
