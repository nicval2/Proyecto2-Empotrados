// audio_test_offsets.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

#define AUDIO_BASE 0x8920

// Probar diferentes configuraciones de offsets
typedef struct {
    char *name;
    int fifo_space_offset;
    int left_offset;
    int right_offset;
} AudioConfig;

AudioConfig configs[] = {
    {"Config 1 (Star Wars)", 4, 8, 12},
    {"Config 2 (Standard)", 4, 2, 3},
    {"Config 3 (Alt 1)", 1, 2, 3},
    {"Config 4 (Alt 2)", 4, 0, 4},
};

void wait_for_space(int offset) {
    volatile unsigned int *fifo = (unsigned int *)(AUDIO_BASE + offset);
    while((*fifo & 0xFF000000) == 0);
}

void test_config(AudioConfig *cfg) {
    volatile unsigned int *audio_left = (unsigned int *)(AUDIO_BASE + cfg->left_offset);
    volatile unsigned int *audio_right = (unsigned int *)(AUDIO_BASE + cfg->right_offset);
    
    printf("\n========================================\n");
    printf("Testing: %s\n", cfg->name);
    printf("  FIFO Space: BASE + %d (0x%08X)\n", 
           cfg->fifo_space_offset, AUDIO_BASE + cfg->fifo_space_offset);
    printf("  Left:       BASE + %d (0x%08X)\n", 
           cfg->left_offset, AUDIO_BASE + cfg->left_offset);
    printf("  Right:      BASE + %d (0x%08X)\n", 
           cfg->right_offset, AUDIO_BASE + cfg->right_offset);
    printf("========================================\n\n");
    
    printf("Playing 440Hz tone for 3 seconds...\n");
    printf("Listen for a clear musical note.\n\n");
    
    int sample_rate = 48000;
    int frequency = 440;
    int duration = 3;
    int total_samples = sample_rate * duration;
    
    int samples_per_cycle = sample_rate / frequency;
    int half_cycle = samples_per_cycle / 2;
    int current_sample = 0;
    
    int volume = 0x08000000; // Volumen del Star Wars
    int wave_value = volume;
    
    for(int i = 0; i < total_samples; i++) {
        // Onda cuadrada simple
        if(current_sample >= half_cycle) {
            wave_value = -volume;
        } else {
            wave_value = volume;
        }
        
        current_sample++;
        if(current_sample >= samples_per_cycle) {
            current_sample = 0;
        }
        
        // Esperar espacio
        wait_for_space(cfg->fifo_space_offset);
        
        // Escribir muestra
        *audio_left = wave_value;
        *audio_right = wave_value;
        
        // Mostrar progreso
        if(i % sample_rate == 0) {
            printf("  %d seconds...\n", i / sample_rate + 1);
        }
    }
    
    printf("Done.\n");
    printf("Did you hear a clear tone? (y/n)\n");
    printf("Press any key to continue to next config...\n\n");
}

int main() {
    printf("\n************************************************\n");
    printf("  Audio Codec Offset Test\n");
    printf("************************************************\n");
    printf("\nThis will test different register configurations.\n");
    printf("Listen for a clear 440Hz musical tone.\n");
    printf("Connect speakers to Line-Out (Green Port).\n\n");
    
    printf("Audio Base Address: 0x%08X\n", AUDIO_BASE);
    
    usleep(2000000); // 2 second pause
    
    // Probar cada configuración
    for(int i = 0; i < 4; i++) {
        test_config(&configs[i]);
        usleep(2000000); // Pausa de 2 segundos entre tests
    }
    
    printf("\n************************************************\n");
    printf("Test complete!\n");
    printf("Which config sounded best?\n");
    printf("************************************************\n\n");
    
    return 0;
}