// audio.h
#ifndef AUDIO_H
#define AUDIO_H
#define NIOS_READY_TOKEN  0xCAFEBABE

#include "alt_types.h"

/* --- Tokens de Protocolo --- */
#define METADATA_MAGIC   0xDEADDA7A
#define AUDIO_EOS_TOKEN  0xFFFFFFFF  // Fin normal
#define AUDIO_SKIP_TOKEN 0xFFFFFFFE  // Skip (next/prev)

/* --- Comandos NIOS -> HPS --- */
#define CMD_NEXT  0x4E455854
#define CMD_PREV  0x50524556

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
int audio_process_sample(void);  // Retorna: 1=OK, 0=EOS, -1=SKIP
void audio_send_command(alt_u32 cmd);
void audio_send_ready_signal(void);

#endif
