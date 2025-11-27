#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>
#include "filter.h"

/* --- DIRECCION BASE DEL AUDIO --- */
// Confirma en system.h si se llama AUDIO_BASE o AUDIO_0_BASE
#ifndef AUDIO_BASE
#define AUDIO_BASE 0x3060
#endif

/* --- DEFINICION DE NOTAS (Frecuencias en Hz) --- */
#define NOTE_G4  392
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_C6  1047

/* --- CONFIGURACION DE AUDIO --- */
#define SAMPLING_RATE 48000
#define VOLUME        0x08AAAAAA // Volumen ajustado para no saturar

/* Funcion para esperar espacio en la FIFO (Offset + 4 bytes) */
void wait_for_fifo_space() {
    volatile unsigned int *audio_fifo_space_ptr = (unsigned int *)(AUDIO_BASE + 4);
    // Esperamos mientras el espacio de escritura del canal izquierdo (bits 24-31) sea 0
    while ( (*audio_fifo_space_ptr & 0xFF000000) == 0 );
}

/* Funcion para reproducir tono */
void play_tone(int freq, int duration_ms) {
    if (freq == 0) {
        usleep(duration_ms * 1000);
        return;
    }

    int samples_per_cycle = SAMPLING_RATE / freq;
    int half_cycle = samples_per_cycle / 2;
    int total_samples = (SAMPLING_RATE * duration_ms) / 1000;

    int current_sample = 0;
    int wave_value = VOLUME;

    // Punteros a los canales Left (Base + 8) y Right (Base + 12)
    volatile unsigned int *audio_left_ptr = (unsigned int *)(AUDIO_BASE + 8);
    volatile unsigned int *audio_right_ptr = (unsigned int *)(AUDIO_BASE + 12);

    for (int i = 0; i < total_samples; i++) {
        if (current_sample >= half_cycle) {
            wave_value = -VOLUME;
        } else {
            wave_value = VOLUME;
        }

        current_sample++;
        if (current_sample >= samples_per_cycle) {
            current_sample = 0;
        }

        // Filtro pasa bajas
        filter_highpass_enable();
        wave_value = filter_highpass_process(wave_value);


        wait_for_fifo_space();
        *audio_left_ptr = wave_value;
        *audio_right_ptr = wave_value;
    }
}

int main() {
    alt_putstr("--- REPRODUCTOR STAR WARS (NIOS II) ---\n");
    alt_putstr("Escuchando en Line-Out (Puerto Verde)...\n");

    while(1) {
        // --- Intro (Tresillos) ---
        play_tone(NOTE_G4, 150); usleep(20000);
        play_tone(NOTE_G4, 150); usleep(20000);
        play_tone(NOTE_G4, 150); usleep(20000);

        // --- Tema Principal (Parte A) ---
        play_tone(NOTE_C5, 1000); // DO (Larga)
        usleep(50000);

        play_tone(NOTE_G5, 1000); // SOL agudo (Larga)
        usleep(50000);

        // Tresillo rapido descendente
        play_tone(NOTE_F5, 150); usleep(10000);
        play_tone(NOTE_E5, 150); usleep(10000);
        play_tone(NOTE_D5, 150); usleep(10000);

        play_tone(NOTE_C6, 1000); // DO muy agudo
        usleep(50000);

        play_tone(NOTE_G5, 500); // SOL agudo
        usleep(50000);

        // Tresillo rapido descendente (Repeticion)
        play_tone(NOTE_F5, 150); usleep(10000);
        play_tone(NOTE_E5, 150); usleep(10000);
        play_tone(NOTE_D5, 150); usleep(10000);

        play_tone(NOTE_C6, 1000); // DO muy agudo
        usleep(50000);

        play_tone(NOTE_G5, 500); // SOL agudo
        usleep(50000);

        // Final de frase
        play_tone(NOTE_F5, 150); usleep(10000);
        play_tone(NOTE_E5, 150); usleep(10000);
        play_tone(NOTE_F5, 150); usleep(10000);

        play_tone(NOTE_D5, 1500); // RE (Final largo)

        // Pausa larga antes de repetir
        usleep(2000000);
    }

    return 0;
}
