// audio_player_nios.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

// Direcciones base (verificar con tu Platform Designer)
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920

// Registros del Audio Core (offsets desde AUDIO_BASE)
#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

// Variables globales
alt_u32 sample_rate = 0;
alt_u32 num_channels = 0;
alt_u32 bits_per_sample = 0;

// Leer registro del Audio Core
alt_u32 audio_read(alt_u32 offset) {
    return IORD_32DIRECT(AUDIO_BASE, offset);
}

// Escribir registro del Audio Core
void audio_write(alt_u32 offset, alt_u32 value) {
    IOWR_32DIRECT(AUDIO_BASE, offset, value);
}

// Esperar espacio en AMBOS FIFOs de audio
void wait_for_audio_space() {
    alt_u32 fifospace;
    alt_u8 wslc, wsrc;  // Write Space Left/Right Channel

    do {
        fifospace = audio_read(AUDIO_FIFOSPACE);
        wslc = (fifospace >> 24) & 0xFF;  // Bits [31:24]
        wsrc = (fifospace >> 16) & 0xFF;  // Bits [23:16]
    } while(wslc == 0 || wsrc == 0);  // Esperar hasta que AMBOS tengan espacio
}

// Enviar sample de audio a ambos canales
void send_audio_sample(alt_16 sample_signed) {
    // El Audio Core espera datos en los bits superiores para 32-bit mode
    // Para Bit Length = 32: los datos van en bits [31:0]
    // El sample de 16 bits debe ir en los bits más significativos

    alt_32 sample_32 = ((alt_32)sample_signed) << 16;

    wait_for_audio_space();

    // Escribir a ambos canales
    audio_write(AUDIO_LEFT_DATA, sample_32);
    audio_write(AUDIO_RIGHT_DATA, sample_32);
}

// Inicializar el Audio Core
void audio_init() {
    // Limpiar FIFOs (escribir 0x0C al control register)
    // Bit 2: Clear Write FIFO
    // Bit 3: Clear Read FIFO
    audio_write(AUDIO_CONTROL, 0x0C);
    usleep(1000);
    audio_write(AUDIO_CONTROL, 0x00);

    printf("Audio Core initialized\n");
    printf("  FIFOSPACE: 0x%08X\n", (unsigned int)audio_read(AUDIO_FIFOSPACE));
}

int main() {
    alt_u32 data, status;
    alt_u32 count = 0;
    alt_u32 seconds = 0;
    int metadata_ok = 0;

    printf("\n========================================\n");
    printf("  NIOS II WAV Player\n");
    printf("========================================\n\n");

    // Inicializar audio
    audio_init();

    printf("Waiting for metadata from ARM...\n");

    // Esperar metadata
    while(!metadata_ok) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            if(data == METADATA_MAGIC) {
                printf("Metadata magic received!\n");

                // Leer sample_rate
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                // Leer num_channels
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                // Leer bits_per_sample
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                printf("  Sample Rate: %u Hz\n", (unsigned int)sample_rate);
                printf("  Channels: %u\n", (unsigned int)num_channels);
                printf("  Bits/Sample: %u\n\n", (unsigned int)bits_per_sample);

                if(sample_rate != 48000) {
                    printf("WARNING: Sample rate mismatch!\n");
                    printf("  File: %u Hz, Codec: 48000 Hz\n\n",
                           (unsigned int)sample_rate);
                }

                metadata_ok = 1;
            }
        }
        usleep(1000);
    }

    printf("Starting playback on Line-Out...\n");
    printf("-----------------------------------\n");

    // Loop principal de reproducción
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            // Convertir de unsigned 16-bit a signed 16-bit
            alt_16 sample_signed = (alt_16)(data & 0xFFFF);

            send_audio_sample(sample_signed);

            count++;

            // Mostrar progreso cada segundo (basado en sample_rate)
            if(count >= sample_rate) {
                seconds++;
                printf("Time: %02u:%02u\n", seconds / 60, seconds % 60);
                count = 0;
            }
        }
    }

    return 0;
}
