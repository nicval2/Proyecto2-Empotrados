// audio_player_nios.c (CORREGIDO - SIN FFLUSH)
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

// Direcciones según Platform Designer
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920  // Audio codec
#define SWITCHES_BASE     0x8850  // Para filtros

#define FIFO_EMPTY (1 << 0)
#define FIFO_FULL  (1 << 1)

// Magic number (debe coincidir con ARM)
#define METADATA_MAGIC 0xDEADDA7A

// Metadata recibida
typedef struct {
    alt_u32 sample_rate;
    alt_u32 num_channels;
    alt_u32 bits_per_sample;
    int metadata_received;
} AudioConfig;

AudioConfig audio_cfg = {0};

// Inicializar Audio Codec
void init_audio_codec() {
    printf("Initializing audio codec...\n");

    // Configuración básica del codec
    // Por ahora asumimos configuración por defecto

    printf("[OK] Audio codec ready\n");
}

// Enviar muestra al codec de audio
void send_to_audio(alt_u32 sample) {
    alt_u32 fifo_space;

    // Verificar espacio en FIFO de audio
    // Registro de status suele estar en offset +4 o +8
    fifo_space = IORD_32DIRECT(AUDIO_BASE + 4, 0);

    // Esperar si está lleno (bit específico depende del IP)
    while((fifo_space & 0xFF000000) == 0) {
        fifo_space = IORD_32DIRECT(AUDIO_BASE + 4, 0);
    }

    // Escribir sample a ambos canales (left y right)
    // Formato: sample en los 16 bits superiores
    IOWR_32DIRECT(AUDIO_BASE, 0, sample << 16);  // Left channel
    IOWR_32DIRECT(AUDIO_BASE, 4, sample << 16);  // Right channel
}

int main() {
    alt_u32 data;
    alt_u32 samples_received = 0;
    alt_u32 status;
    alt_u32 last_print = 0;

    printf("\n========================================\n");
    printf("  NIOS II Audio Player\n");
    printf("========================================\n\n");

    // Inicializar codec de audio
    init_audio_codec();

    printf("Waiting for metadata from ARM...\n");
    printf("-----------------------------------\n");

    // Recibir metadata
    while(!audio_cfg.metadata_received) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            if(data == METADATA_MAGIC) {
                printf("Metadata received! Magic: 0x%08X\n", (unsigned int)data);

                // Leer sample rate
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY) {
                    usleep(100);
                }
                audio_cfg.sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                // Leer channels
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY) {
                    usleep(100);
                }
                audio_cfg.num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                // Leer bits per sample
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY) {
                    usleep(100);
                }
                audio_cfg.bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                audio_cfg.metadata_received = 1;

                printf("\nAudio Configuration:\n");
                printf("  Sample Rate: %u Hz\n", (unsigned int)audio_cfg.sample_rate);
                printf("  Channels: %u\n", (unsigned int)audio_cfg.num_channels);
                printf("  Bits/Sample: %u\n\n", (unsigned int)audio_cfg.bits_per_sample);
            }
        }
        usleep(1000);
    }

    printf("Starting playback...\n");
    printf("-----------------------------------\n");

    // Loop principal de reproducción
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

        if(!(status & FIFO_EMPTY)) {
            // Leer sample del FIFO
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

            // Enviar al codec de audio
            send_to_audio(data);

            samples_received++;

            // Mostrar progreso cada segundo (sin fflush)
            if(samples_received % audio_cfg.sample_rate == 0) {
                if(samples_received != last_print) {
                    printf("Playing: %u seconds\n",
                           (unsigned int)(samples_received / audio_cfg.sample_rate));
                    last_print = samples_received;
                }
            }
        } else {
            // FIFO vacío, pequeña espera
            usleep(100);
        }
    }

    return 0;
}
