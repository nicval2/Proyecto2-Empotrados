// audio.h - Cabecera optimizada Bare Metal
#ifndef AUDIO_H
#define AUDIO_H

/* Definiciones de Tokens */
#define NIOS_READY_TOKEN  0xCAFEBABE
#define METADATA_MAGIC    0xDEADDA7A
#define AUDIO_EOS_TOKEN   0xFFFFFFFF
#define AUDIO_SKIP_TOKEN  0xFFFFFFFE

/* Comandos */
#define CMD_NEXT  0x4E455854
#define CMD_PREV  0x50524556

/* Variables Globales */
extern char artist_str[64];
extern char album_str[64];
extern char title_str[64];
extern int volume_shift;

/* Prototipos de Funciones (Tipos actualizados a unsigned int) */
void audio_init(void);
void audio_wait_handshake(void);

/* AQUÍ ESTABA EL ERROR: Cambiado de alt_u32 a unsigned int */
unsigned int audio_receive_metadata(void);

int audio_fifo_has_data(void);
int audio_process_sample(void);

/* AQUÍ TAMBIÉN: Cambiado de alt_u32 a unsigned int */
void audio_send_command(unsigned int cmd);

void audio_send_ready_signal(void);

#endif
