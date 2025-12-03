// audio.c - Manejo de audio Bare Metal
#include "system.h"
#include "audio.h"
#include "filter.h"

/* Definición de Punteros Directos a Hardware */
#define FIFO_OUT_PTR      ((volatile unsigned int *)0x6870)
#define FIFO_OUT_CSR_PTR  ((volatile unsigned int *)0x6880)

#define FIFO2_IN_PTR      ((volatile unsigned int *)0x6950)
#define FIFO2_IN_CSR_PTR  ((volatile unsigned int *)0x6980)

#define AUDIO_PTR         ((volatile unsigned int *)0x6920)

/* Offsets (en palabras de 32 bits, por eso indices 0, 1, etc) */
#define AUDIO_REG_CONTROL    0
#define AUDIO_REG_FIFOSPACE  1
#define AUDIO_REG_LEFT       2
#define AUDIO_REG_RIGHT      3

#define CSR_FILL_LEVEL_IDX   0
// Nota: Dependiendo de la implementación del FIFO CSR, el fill level suele estar en offset 0 o 1.
// Asumiendo offset 0 basado en IORD_32DIRECT(BASE, 0) del codigo original.

/* Variables Globales */
char artist_str[64];
char album_str[64];
char title_str[64];
int volume_shift = 0;

/* Retardo interno simple */
static void audio_delay(int loops) {
    volatile int i;
    for(i=0; i<loops; i++);
}

/* Funciones Privadas */
static unsigned int fifo_out_fill_level(void) {
    return FIFO_OUT_CSR_PTR[CSR_FILL_LEVEL_IDX];
}

static unsigned int fifo2_in_fill_level(void) {
    return FIFO2_IN_CSR_PTR[CSR_FILL_LEVEL_IDX];
}

static void receive_string_helper(char *buffer, int max_len) {
    unsigned int len;

    while(fifo_out_fill_level() == 0);
    len = *FIFO_OUT_PTR;

    if(len >= max_len) len = max_len - 1;

    for(int i = 0; i < len; i++) {
        while(fifo_out_fill_level() == 0);
        buffer[i] = (char)(*FIFO_OUT_PTR);
    }
    buffer[len] = '\0';
}

/* Funciones Publicas */

void audio_init(void) {
    // Reset FIFOs del codec escribiendo en registro de control
    AUDIO_PTR[AUDIO_REG_CONTROL] = 0x0C; // Clear
    audio_delay(1000);
    AUDIO_PTR[AUDIO_REG_CONTROL] = 0x00;
}

void audio_wait_handshake(void) {
    unsigned int data;
    while(1) {
        if(fifo_out_fill_level() > 0) {
            data = *FIFO_OUT_PTR;
            if(data == METADATA_MAGIC) {
                break;
            }
        }
    }
}

unsigned int audio_receive_metadata(void) {
    unsigned int sample_rate, num_channels, bits;

    while(fifo_out_fill_level() == 0);
    sample_rate = *FIFO_OUT_PTR;

    while(fifo_out_fill_level() == 0);
    num_channels = *FIFO_OUT_PTR;

    while(fifo_out_fill_level() == 0);
    bits = *FIFO_OUT_PTR;

    receive_string_helper(artist_str, 64);
    receive_string_helper(album_str, 64);
    receive_string_helper(title_str, 64);

    return sample_rate;
}

void audio_send_ready_signal(void) {
    // Esperar espacio en FIFO2 (salida)
    while(fifo2_in_fill_level() >= 15);
    *FIFO2_IN_PTR = NIOS_READY_TOKEN;
}

int audio_fifo_has_data(void) {
    return (fifo_out_fill_level() > 0);
}

int audio_process_sample(void) {
    // Leer dato crudo del FIFO
    unsigned int raw_data = *FIFO_OUT_PTR;

    // Verificar tokens
    if (raw_data == AUDIO_EOS_TOKEN) return 0;
    if (raw_data == AUDIO_SKIP_TOKEN) return -1;

    // Casting explicito para manejo de signo
    short sample_16 = (short)(raw_data & 0xFFFF);

    // Filtrado
    short filtered_16 = filter_process(sample_16);

    // Expansión a 32 bits para el códec y ajuste de volumen
    int sample_32 = ((int)filtered_16) << 16;
    sample_32 = sample_32 >> volume_shift;

    // Esperar espacio en el Audio Core
    // El registro FIFOSPACE contiene: [WSLC (8) | WSRC (8) | RALC (8) | RARC (8)]
    // Nos interesa Write Space Left/Right (bytes altos)
    unsigned int fifospace;
    do {
        fifospace = AUDIO_PTR[AUDIO_REG_FIFOSPACE];
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);

    // Escribir a canales L y R
    AUDIO_PTR[AUDIO_REG_LEFT] = sample_32;
    AUDIO_PTR[AUDIO_REG_RIGHT] = sample_32;

    return 1;
}

void audio_send_command(unsigned int cmd) {
    while(fifo2_in_fill_level() >= 15);
    *FIFO2_IN_PTR = cmd;
}
