// audio_test_diagnostic.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

#define AUDIO_BASE        0x8920
#define AUDIO_FIFO_SPACE  (AUDIO_BASE + 4)
#define AUDIO_LEFT_DATA   (AUDIO_BASE + 8)
#define AUDIO_RIGHT_DATA  (AUDIO_BASE + 12)

void wait_for_fifo_space() {
    volatile unsigned int *fifo_space = (unsigned int *)AUDIO_FIFO_SPACE;
    while((*fifo_space & 0xFF000000) == 0);
}

void test_tone_with_sample_rate(int sample_rate, int frequency, int duration_sec) {
    volatile unsigned int *audio_left = (unsigned int *)AUDIO_LEFT_DATA;
    volatile unsigned int *audio_right = (unsigned int *)AUDIO_RIGHT_DATA;
    
    printf("\n========================================\n");
    printf("Test Parameters:\n");
    printf("  Sample Rate: %d Hz\n", sample_rate);
    printf("  Frequency:   %d Hz\n", frequency);
    printf("  Duration:    %d seconds\n", duration_sec);
    printf("========================================\n\n");
    
    int total_samples = sample_rate * duration_sec;
    int samples_per_cycle = sample_rate / frequency;
    int half_cycle = samples_per_cycle / 2;
    
    printf("Calculated:\n");
    printf("  Total samples: %d\n", total_samples);
    printf("  Samples per cycle: %d\n", samples_per_cycle);
    printf("  Half cycle: %d\n\n", half_cycle);
    
    printf("Playing tone...\n");
    
    int current_sample = 0;
    unsigned int volume = 0x08000000;  // Volumen medio
    int wave_value;
    
    for(int i = 0; i < total_samples; i++) {
        // Onda cuadrada
        if(current_sample < half_cycle) {
            wave_value = volume;
        } else {
            wave_value = -volume;
        }
        
        current_sample++;
        if(current_sample >= samples_per_cycle) {
            current_sample = 0;
        }
        
        wait_for_fifo_space();
        *audio_left = wave_value;
        *audio_right = wave_value;
        
        // Mostrar progreso
        if(i % sample_rate == 0) {
            printf("  Second %d/%d\n", i / sample_rate + 1, duration_sec);
        }
    }
    
    printf("Done!\n\n");
}

int main() {
    printf("\n************************************************\n");
    printf("  Audio Sample Rate Diagnostic\n");
    printf("************************************************\n\n");
    
    printf("Audio registers:\n");
    printf("  FIFO Space: 0x%08X = 0x%08X\n", 
           AUDIO_FIFO_SPACE, 
           (unsigned int)IORD_32DIRECT(AUDIO_FIFO_SPACE, 0));
    
    printf("\nWe will test different sample rates.\n");
    printf("Each tone should last EXACTLY 5 seconds.\n");
    printf("Note which one sounds correct.\n\n");
    
    printf("Press ENTER to start...\n");
    usleep(3000000); // 3 sec pause
    
    // Test 1: 48000 Hz (configurado)
    printf("\n[Test 1] Testing 48000 Hz (configured rate)\n");
    test_tone_with_sample_rate(48000, 440, 5);
    usleep(2000000);
    
    // Test 2: 96000 Hz (el doble)
    printf("\n[Test 2] Testing 96000 Hz (2x configured)\n");
    test_tone_with_sample_rate(96000, 440, 5);
    usleep(2000000);
    
    // Test 3: 24000 Hz (la mitad)
    printf("\n[Test 3] Testing 24000 Hz (0.5x configured)\n");
    test_tone_with_sample_rate(24000, 440, 5);
    usleep(2000000);
    
    // Test 4: 44100 Hz
    printf("\n[Test 4] Testing 44100 Hz (CD quality)\n");
    test_tone_with_sample_rate(44100, 440, 5);
    
    printf("\n************************************************\n");
    printf("Which test lasted exactly 5 seconds?\n");
    printf("1 = 48000 Hz\n");
    printf("2 = 96000 Hz\n");
    printf("3 = 24000 Hz\n");
    printf("4 = 44100 Hz\n");
    printf("************************************************\n\n");
    
    return 0;
}