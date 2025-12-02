//audio.c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "system.h"
#include "io.h"
#include "audio.h"
#include "filter.h"  // Necesario para aplicar el filtro dentro de process_sample

/* --- DIRECCIONES DE HARDWARE (Privadas de este archivo) --- */
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920

#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

/* --- Variables Globales --- */
char artist_str[64];
char album_str[64];
char title_str[64];
int volume_shift = 0; // 0 = Max vol, 2 = Bajo

/* --- Funciones Privadas (Helpers) --- */

// Función interna para leer strings de la FIFO
static void receive_string_helper(char *buffer, int max_len) {
    alt_u32 len;
    int i;

    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    len = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    // printf eliminado aquí para no ralentizar

    if(len >= (alt_u32)max_len) len = max_len - 1;

    for(i = 0; i < (int)len; i++) {
        while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
        buffer[i] = (char)IORD_32DIRECT(FIFO_OUT_BASE, 0);
    }
    buffer[len] = '\0';
}

/* --- Funciones Públicas --- */

void audio_init(void) {
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C); // Reset FIFOs
    usleep(1000);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);
}

void audio_wait_handshake(void) {
    alt_u32 data, status;
    printf("Esperando HPS (Magic Token)...\n");
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            if(data == METADATA_MAGIC) {
                printf("Sincronizado!\n");
                break;
            }
        }
    }
}

alt_u32 audio_receive_metadata(void) {
    alt_u32 sample_rate, num_channels, bits_per_sample;

    // Recibir todo SIN printf para no ralentizar
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    receive_string_helper(artist_str, 64);
    receive_string_helper(album_str, 64);
    receive_string_helper(title_str, 64);

    // Ahora sí imprimir (después de recibir todo)
    printf("Metadata received:\n");
    printf("  Rate: %lu Hz, Channels: %lu, Bits: %lu\n",
           sample_rate, num_channels, bits_per_sample);
    printf("  Artist: %s\n", artist_str);
    printf("  Album:  %s\n", album_str);
    printf("  Title:  %s\n", title_str);

    return sample_rate;
}

int audio_fifo_has_data(void) {
    alt_u32 status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
    return !(status & FIFO_EMPTY);
}

void audio_process_sample(void) {
    // 1. Leer dato de la FIFO
    alt_u32 raw_data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    // 2. Convertir a 16 bits con signo
    alt_16 sample_16 = (alt_16)(raw_data & 0xFFFF);

    // 3. APLICAR FILTRO (Desde filter.h)
    // El filtro sabe cuál usar gracias a filter_set() que llamamos en main
    alt_16 filtered_16 = filter_process(sample_16);

    // 4. Extender a 32 bits y alinear MSB
    alt_32 sample_32 = ((alt_32)filtered_16) << 16;

    // 5. APLICAR VOLUMEN (Shift)
    sample_32 = sample_32 >> volume_shift;

    // 6. Esperar espacio en Hardware de Audio
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);

    // 7. Escribir
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);
}
