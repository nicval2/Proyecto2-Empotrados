// audio_test_starwars_style.c
#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>

#define AUDIO_BASE 0x8920
#define SAMPLING_RATE 48000
#define VOLUME 0x08AAAAAA

void wait_for_fifo_space() {
    volatile unsigned int *audio_fifo_space_ptr = (unsigned int *)(AUDIO_BASE + 4);
    while((*audio_fifo_space_ptr & 0xFF000000) == 0);
}

void play_tone(int freq, int duration_ms) {
    if(freq == 0) {
        usleep(duration_ms * 1000);
        return;
    }

    int samples_per_cycle = SAMPLING_RATE / freq;
    int half_cycle = samples_per_cycle / 2;
    int total_samples = (SAMPLING_RATE * duration_ms) / 1000;

    int current_sample = 0;
    int wave_value = VOLUME;

    volatile unsigned int *audio_left_ptr = (unsigned int *)(AUDIO_BASE + 8);
    volatile unsigned int *audio_right_ptr = (unsigned int *)(AUDIO_BASE + 12);

    for(int i = 0; i < total_samples; i++) {
        if(current_sample >= half_cycle) {
            wave_value = -VOLUME;
        } else {
            wave_value = VOLUME;
        }

        current_sample++;
        if(current_sample >= samples_per_cycle) {
            current_sample = 0;
        }

        wait_for_fifo_space();
        *audio_left_ptr = wave_value;
        *audio_right_ptr = wave_value;
    }
}

int main() {
    alt_putstr("Testing with Star Wars exact code...\n");
    alt_putstr("Playing 440Hz tone...\n");
    
    // Reproducir un tono simple de 5 segundos
    play_tone(440, 5000);
    
    alt_putstr("Done!\n");
    
    return 0;
}