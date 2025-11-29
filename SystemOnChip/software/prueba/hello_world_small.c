#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920

#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

alt_u32 sample_rate = 0;
alt_u32 num_channels = 0;
alt_u32 bits_per_sample = 0;

// Esperar espacio en el FIFO de audio (canal izquierdo)
void wait_for_left_space() {
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0);
}

// Esperar espacio en el FIFO de audio (canal derecho)
void wait_for_right_space() {
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 16) & 0xFF) == 0);
}

// Inicializar el Audio Core
void audio_init() {
    // Limpiar FIFOs
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
    usleep(1000);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);

    printf("Audio initialized\n");
}

int main() {
    alt_u32 data, status;
    alt_u32 count = 0;
    alt_u32 seconds = 0;
    int metadata_ok = 0;

    printf("\n========================================\n");
    printf("  NIOS II WAV Player v2\n");
    printf("========================================\n\n");

    audio_init();

    printf("Waiting for metadata...\n");

    // Esperar metadata
    while(!metadata_ok) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            if(data == METADATA_MAGIC) {
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                printf("Sample Rate: %u Hz\n", (unsigned int)sample_rate);
                printf("Channels: %u\n", (unsigned int)num_channels);
                printf("Bits: %u\n\n", (unsigned int)bits_per_sample);

                metadata_ok = 1;
            }
        }
        usleep(1000);
    }

    printf("Playing...\n");

    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            // Convertir a signed y escalar a 32 bits
            alt_16 sample_signed = (alt_16)(data & 0xFFFF);
            alt_32 sample_32 = ((alt_32)sample_signed) << 16;

            // Escribir a LEFT, esperar, escribir a RIGHT
            wait_for_left_space();
            IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);

            wait_for_right_space();
            IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);

            count++;

            if(count >= sample_rate) {
                seconds++;
                printf("Time: %02u:%02u\n", seconds / 60, seconds % 60);
                count = 0;
            }
        }
    }

    return 0;
}
