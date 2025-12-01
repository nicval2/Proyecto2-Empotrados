// audio.c
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "system.h"
#include "io.h"
#include "audio.h"
#include "filter.h"

/* --- DIRECCIONES DE HARDWARE --- */
// FIFO: HPS -> NIOS (lectura)
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880

// FIFO2: NIOS -> HPS (escritura)
#define FIFO2_IN_BASE     0x8950
#define FIFO2_IN_CSR_BASE 0x8A00

// Audio Codec
#define AUDIO_BASE        0x8920
#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

/* --- Flags de Status --- */
#define FIFO_EMPTY (1 << 0)
#define FIFO_FULL  (1 << 1)

/* --- Variables Globales --- */
char artist_str[64];
char album_str[64];
char title_str[64];
int volume_shift = 0;

/* --- Funciones Privadas --- */

static void receive_string_helper(char *buffer, int max_len) {
    alt_u32 len;

    // Leer longitud
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    len = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    if(len >= max_len) len = max_len - 1;

    // Leer caracteres
    for(int i = 0; i < len; i++) {
        while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
        buffer[i] = (char)IORD_32DIRECT(FIFO_OUT_BASE, 0);
    }
    buffer[len] = '\0';
}

/* --- Funciones Públicas --- */

void audio_init(void) {
    // Reset FIFOs del codec de audio
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
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
        usleep(1000);
    }
}

alt_u32 audio_receive_metadata(void) {
    alt_u32 sample_rate, num_channels, bits_per_sample;

    // Recibir configuración numérica
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    // Recibir strings de metadata
    receive_string_helper(artist_str, 64);
    receive_string_helper(album_str, 64);
    receive_string_helper(title_str, 64);

    printf("Metadata: %u Hz, %lu ch, %lu bits\n",
           (unsigned)sample_rate, num_channels, bits_per_sample);

    return sample_rate;
}

int audio_fifo_has_data(void) {
    alt_u32 status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
    return !(status & FIFO_EMPTY);
}

int audio_process_sample(void) {
    // 1. Leer dato de la FIFO
    alt_u32 raw_data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

    // 2. Verificar tokens especiales
    if (raw_data == AUDIO_EOS_TOKEN) {
        return 0;  // Fin de canción normal
    }
    if (raw_data == AUDIO_SKIP_TOKEN) {
        return -1; // Salto forzado (next/prev)
    }

    // 3. Convertir a 16 bits con signo
    alt_16 sample_16 = (alt_16)(raw_data & 0xFFFF);

    // 4. Aplicar filtro
    alt_16 filtered_16 = filter_process(sample_16);

    // 5. Extender a 32 bits y alinear para el codec
    alt_32 sample_32 = ((alt_32)filtered_16) << 16;

    // 6. Aplicar volumen
    sample_32 = sample_32 >> volume_shift;

    // 7. Esperar espacio en el codec de audio
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);

    // 8. Escribir al codec (mono -> ambos canales)
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);

    return 1; // Éxito
}

/* === NUEVAS FUNCIONES === */

void audio_send_command(alt_u32 cmd) {
    // Esperar a que haya espacio en FIFO2
    while(IORD_32DIRECT(FIFO2_IN_CSR_BASE, 0) & FIFO_FULL);

    // Escribir comando
    IOWR_32DIRECT(FIFO2_IN_BASE, 0, cmd);

    printf("CMD enviado: 0x%08X\n", (unsigned)cmd);
}

void audio_flush_fifo(void) {
    // Vaciar el FIFO descartando datos hasta encontrar EOS o SKIP
    // Esto es necesario cuando el HPS va a cambiar de canción
    alt_u32 data;
    int count = 0;

    printf("Vaciando FIFO...\n");

    while(1) {
        // Si no hay datos, esperar un poco y reintentar
        if(!(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            count++;

            // Si encontramos token de fin, salir
            if(data == AUDIO_EOS_TOKEN || data == AUDIO_SKIP_TOKEN) {
                printf("FIFO vaciado (%d muestras descartadas)\n", count);
                break;
            }
        } else {
            // FIFO vacío, dar tiempo al HPS
            usleep(100);
        }

        // Timeout de seguridad (evitar bucle infinito)
        if(count > 100000) {
            printf("Timeout vaciando FIFO\n");
            break;
        }
    }
}
