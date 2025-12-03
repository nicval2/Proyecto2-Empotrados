#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "system.h"
#include "io.h"
#include "audio.h"
#include "filter.h"

/* --- Variables Globales --- */
char artist_str[64];
char album_str[64];
char title_str[64];
int volume_shift = 0;

/* --- Funciones Privadas --- */

// Leer fill level del FIFO (HPS->NIOS)
static alt_u32 fifo_out_fill_level(void) {
    return IORD_32DIRECT(FIFO_OUT_CSR_BASE, CSR_FILL_LEVEL);
}

// Leer fill level del FIFO2 (NIOS->HPS)
static alt_u32 fifo2_in_fill_level(void) {
    return IORD_32DIRECT(FIFO2_IN_CSR_BASE, CSR_FILL_LEVEL);
}

void audio_send_ready_signal(void) {
    // Esperar espacio en FIFO2
    while(fifo2_in_fill_level() >= 15);

    // Enviar token
    IOWR_32DIRECT(FIFO2_IN_BASE, 0, NIOS_READY_TOKEN);
    printf("READY enviado al HPS\n");
}

static void receive_string_helper(char *buffer, int max_len) {
    alt_u32 len;

    // Esperar y leer longitud
    while(fifo_out_fill_level() == 0);
    len = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    if(len >= max_len) len = max_len - 1;

    // Leer caracteres
    for(int i = 0; i < len; i++) {
        while(fifo_out_fill_level() == 0);
        buffer[i] = (char)IORD_32DIRECT(FIFO_OUT_BASE, 0);
    }
    buffer[len] = '\0';
}

/* --- Funciones Publicas --- */
void audio_init(void) {
    // Reset FIFOs del codec
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
    usleep(1000);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);
}

void audio_wait_handshake(void) {
    alt_u32 data;
    printf("Esperando HPS...\n");

    while(1) {
        if(fifo_out_fill_level() > 0) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            if(data == METADATA_MAGIC) {
                printf("Sincronizado!\n");
                break;
            }
        }
        usleep(1000);
    }
}

alt_u32 audio_receive_metadata(void) {
    alt_u32 sample_rate, num_channels, bits_per_sample;

    // Esperar y leer sample rate
    while(fifo_out_fill_level() == 0);
    sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(fifo_out_fill_level() == 0);
    num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(fifo_out_fill_level() == 0);
    bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    // Leer strings
    receive_string_helper(artist_str, 64);
    receive_string_helper(album_str, 64);
    receive_string_helper(title_str, 64);

    printf("Rate: %u Hz, Ch: %lu\n", (unsigned)sample_rate, num_channels);

    return sample_rate;
}

int audio_fifo_has_data(void) {
    return fifo_out_fill_level() > 0;
}

int audio_process_sample(void) {
    // Leer dato
    alt_u32 raw_data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    // Verificar tokens especiales
    if (raw_data == AUDIO_EOS_TOKEN) {
        return 0;  // Fin normal
    }
    if (raw_data == AUDIO_SKIP_TOKEN) {
        return -1; // Skip
    }

    // Procesar audio
    alt_16 sample_16 = (alt_16)(raw_data & 0xFFFF);
    alt_16 filtered_16 = filter_process(sample_16);
    alt_32 sample_32 = ((alt_32)filtered_16) << 16;
    sample_32 = sample_32 >> volume_shift;

    // Esperar espacio en codec
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);

    // Escribir a ambos canales
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);

    return 1;
}

void audio_send_command(alt_u32 cmd) {
    // Esperar espacio en FIFO2 (capacidad 16, esperar si fill >= 15)
    while(fifo2_in_fill_level() >= 15);

    // Escribir comando
    IOWR_32DIRECT(FIFO2_IN_BASE, 0, cmd);
}
