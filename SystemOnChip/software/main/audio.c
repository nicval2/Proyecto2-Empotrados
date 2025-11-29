#include "audio.h"
#include "filter.h"
#include "buttons.h" // Para acceder a is_running
#include <stdio.h>
#include <system.h>
#include <io.h>

// Definiciones de Hardware
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920

#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

// Estado interno
typedef enum {
    STATE_WAIT_MAGIC,
    STATE_READ_RATE,
    STATE_READ_CHANNELS,
    STATE_READ_BITS,
    STATE_PLAYING
} AudioState;

static AudioState current_state = STATE_WAIT_MAGIC;
static alt_u32 sample_rate = 0;
static alt_u32 num_channels = 0;
static alt_u32 bits_per_sample = 0;
static int volume_shift = 0; // 0 es volumen máximo

// Inicialización
void audio_init() {
    // Limpiar FIFOs del Audio Core
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
    // Pequeño delay simulado (evitamos usleep largo para no bloquear)
    for(volatile int i=0; i<5000; i++);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);

    current_state = STATE_WAIT_MAGIC;
    printf("[Audio] Initialized. Waiting for stream...\n");
}

// Configurar volumen (atenuación por desplazamiento de bits)
void audio_set_volume(int shift_amount) {
    if (shift_amount < 0) shift_amount = 0;
    if (shift_amount > 8) shift_amount = 8; // Límite para no silenciar todo
    volume_shift = shift_amount;
}

// Función NO BLOQUEANTE llamada desde el main loop
void audio_poll() {
    // 1. Si está en PAUSA (controlado por botones), no hacemos nada
    if (!is_running && current_state == STATE_PLAYING) {
        return;
    }

    alt_u32 fifo_status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);

    // Si el FIFO de entrada está vacío, no tenemos datos para procesar, retornamos.
    if (fifo_status & FIFO_EMPTY) {
        return;
    }

    alt_u32 incoming_data;

    switch (current_state) {
        case STATE_WAIT_MAGIC:
            incoming_data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            if (incoming_data == METADATA_MAGIC) {
                current_state = STATE_READ_RATE;
            }
            break;

        case STATE_READ_RATE:
            sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            current_state = STATE_READ_CHANNELS;
            break;

        case STATE_READ_CHANNELS:
            num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            current_state = STATE_READ_BITS;
            break;

        case STATE_READ_BITS:
            bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            printf("[Audio] Metadata OK. Rate: %u, Ch: %u, Bits: %u\n",
                   (unsigned int)sample_rate, (unsigned int)num_channels, (unsigned int)bits_per_sample);
            current_state = STATE_PLAYING;
            break;

        case STATE_PLAYING:
            // Verificamos espacio en el Audio Core ANTES de leer el dato
            // Para no perder el dato si el audio core está lleno.
            {
                alt_u32 audio_space = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
                int left_space = (audio_space >> 24) & 0xFF;
                int right_space = (audio_space >> 16) & 0xFF;

                // Si no hay espacio en alguno de los canales, retornamos y probamos luego
                if (left_space == 0 || right_space == 0) {
                    return;
                }

                // LEER dato del FIFO (Origen)
                incoming_data = IORD_32DIRECT(FIFO_OUT_BASE, 0);

                // 1. Convertir a signed 16-bit
                alt_16 sample_16 = (alt_16)(incoming_data & 0xFFFF);

                // 2. APLICAR FILTRO (Tu modulo filter.c)
                sample_16 = filter_process(sample_16);

                // 3. APLICAR VOLUMEN (Shift right divide por 2^n)
                sample_16 = sample_16 >> volume_shift;

                // 4. Escalar a 32 bits para el Audio Core
                alt_32 sample_32 = ((alt_32)sample_16) << 16;

                // 5. Escribir al Audio Core
                IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
                IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);
            }
            break;
    }
}
