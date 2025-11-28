// audio_test_tone.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>
#include <math.h>

#define AUDIO_BASE        0x8920
#define AUDIO_FIFO_SPACE  (AUDIO_BASE + 4)
#define AUDIO_LEFT_DATA   (AUDIO_BASE + 8)
#define AUDIO_RIGHT_DATA  (AUDIO_BASE + 12)

#define SAMPLE_RATE 48000
#define FREQUENCY 440  // La (A4)
#define VOLUME 0x4000  // Volumen medio (16-bit signed)

void wait_for_audio_fifo() {
    volatile alt_u32 *fifo_space = (alt_u32 *)AUDIO_FIFO_SPACE;
    while((*fifo_space & 0xFF000000) == 0);
}

void play_test_tone(int duration_seconds) {
    volatile alt_u32 *audio_left = (alt_u32 *)AUDIO_LEFT_DATA;
    volatile alt_u32 *audio_right = (alt_u32 *)AUDIO_RIGHT_DATA;
    
    int total_samples = SAMPLE_RATE * duration_seconds;
    int samples_per_cycle = SAMPLE_RATE / FREQUENCY;
    
    printf("Playing %dHz tone for %d seconds...\n", FREQUENCY, duration_seconds);
    printf("Samples per cycle: %d\n", samples_per_cycle);
    
    for(int i = 0; i < total_samples; i++) {
        // Generar onda sinusoidal
        float t = (float)i / SAMPLE_RATE;
        float sine = sin(2.0 * 3.14159265359 * FREQUENCY * t);
        
        // Convertir a 16-bit signed
        alt_16 sample_16 = (alt_16)(sine * VOLUME);
        
        // Left Justified: sample en bits [31:16]
        alt_32 sample_32 = ((alt_32)sample_16) << 16;
        
        wait_for_audio_fifo();
        *audio_left = sample_32;
        *audio_right = sample_32;
        
        if(i % SAMPLE_RATE == 0) {
            printf("  %d seconds...\n", i / SAMPLE_RATE);
        }
    }
    
    printf("Done!\n");
}

int main() {
    printf("\n========================================\n");
    printf("  Audio Codec Test - Tone Generator\n");
    printf("========================================\n\n");
    
    printf("Audio addresses:\n");
    printf("  FIFO Space: 0x%08X\n", AUDIO_FIFO_SPACE);
    printf("  Left Data:  0x%08X\n", AUDIO_LEFT_DATA);
    printf("  Right Data: 0x%08X\n\n", AUDIO_RIGHT_DATA);
    
    printf("Testing audio output...\n");
    printf("Listen on Line-Out (Green Port)\n\n");
    
    // Reproducir tono de 5 segundos
    play_test_tone(5);
    
    printf("\nTest complete.\n");
    printf("If you heard a clear 440Hz tone, the codec works!\n");
    
    return 0;
}