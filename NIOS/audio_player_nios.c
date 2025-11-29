#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>
#include <string.h>

// Direcciones Base (Ajustar según system.h)
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920

#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

// Buffers para strings
char artist_str[64];
char album_str[64];
char title_str[64];

// Función para leer un string de la FIFO
void receive_string_from_fifo(char *buffer, int max_len) {
    alt_u32 len;
    
    // 1. Leer longitud
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    len = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    if(len >= max_len) len = max_len - 1; // Protección overflow
    
    // 2. Leer caracteres
    for(int i = 0; i < len; i++) {
        while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
        buffer[i] = (char)IORD_32DIRECT(FIFO_OUT_BASE, 0);
    }
    buffer[len] = '\0'; // Terminador null
}

// ... (Funciones de audio init y send_sample iguales al código anterior) ...
void audio_init() {
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
    usleep(1000);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);
}

void send_audio_sample(alt_u32 sample) {
    alt_16 sample_signed = (alt_16)(sample & 0xFFFF);
    alt_32 sample_32 = ((alt_32)sample_signed) << 16;
    
    // Esperar espacio en audio core
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);

    IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);
}

int main() {
    alt_u32 data, status;
    alt_u32 sample_rate, num_channels, bits_per_sample;
    alt_u32 count = 0;
    alt_u32 seconds = 0;
    
    printf("\n--- NIOS II PLAYER V2 (With Metadata) ---\n");
    audio_init();
    
    // 1. Sincronización MAGIC
    printf("Waiting for connection...\n");
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            if(data == METADATA_MAGIC) break;
        }
    }
    
    // 2. Recibir Config Audio
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    // 3. RECIBIR METADATA TEXTO (Nuevo)
    receive_string_from_fifo(artist_str, 64);
    receive_string_from_fifo(album_str, 64);
    receive_string_from_fifo(title_str, 64);
    
    // 4. Mostrar Info en Consola
    printf("\nNow Playing:\n");
    printf("Artist:  %s\n", artist_str);
    printf("Album:   %s\n", album_str);
    printf("Title:   %s\n", title_str);
    printf("Format:  %ld Hz / %ld bit\n", sample_rate, bits_per_sample);
    printf("-------------------------------\n");
    
    // 5. Reproducir Audio y Mostrar Tiempo
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            send_audio_sample(data);
            
            count++;
            if(count >= sample_rate) {
                seconds++;
                // Imprimir tiempo sobreescribiendo la línea anterior (\r)
                printf("\rTime: %02ld:%02ld", seconds / 60, seconds % 60);
                count = 0;
            }
        }
    }
    return 0;
}