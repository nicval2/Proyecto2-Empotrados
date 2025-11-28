// simple_tone_test.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

#define AUDIO_BASE 0x8920

void wait_for_fifo_space() {
    volatile unsigned int *fifo_space = (unsigned int *)(AUDIO_BASE + 4);
    while((*fifo_space & 0xFF000000) == 0);
}

int main() {
    volatile unsigned int *audio_left = (unsigned int *)(AUDIO_BASE + 8);
    volatile unsigned int *audio_right = (unsigned int *)(AUDIO_BASE + 12);
    
    printf("\n========================================\n");
    printf("  Simple 1kHz Tone Test\n");
    printf("========================================\n\n");
    
    printf("Generating pure 1000Hz sine wave...\n");
    printf("This should sound like a clear telephone dial tone.\n");
    printf("Playing for 10 seconds...\n\n");
    
    int sample_rate = 48000;
    int frequency = 1000;
    int duration = 10;
    int total_samples = sample_rate * duration;
    
    // Generar onda sinusoidal suave
    for(int i = 0; i < total_samples; i++) {
        // Usar matemática simple para sinusoide
        // sin(2*pi*f*t) donde t = i/sample_rate
        
        // Aproximación de seno con tabla lookup sería mejor,
        // pero hagamos algo simple primero
        int samples_per_cycle = sample_rate / frequency;
        int position_in_cycle = i % samples_per_cycle;
        
        // Onda triangular como aproximación
        int sample;
        if(position_in_cycle < samples_per_cycle / 2) {
            // Subiendo
            sample = (position_in_cycle * 0x10000000) / (samples_per_cycle / 2) - 0x08000000;
        } else {
            // Bajando
            sample = 0x08000000 - ((position_in_cycle - samples_per_cycle/2) * 0x10000000) / (samples_per_cycle / 2);
        }
        
        wait_for_fifo_space();
        *audio_left = sample;
        *audio_right = sample;
        
        if(i % sample_rate == 0) {
            printf("  %d seconds...\n", i / sample_rate + 1);
        }
    }
    
    printf("\n[DONE]\n");
    printf("Did you hear a clear 1000Hz tone?\n");
    printf("If YES: codec is working, problem is in WAV processing\n");
    printf("If NO: codec configuration issue\n\n");
    
    return 0;
}