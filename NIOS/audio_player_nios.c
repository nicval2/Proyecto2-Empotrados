// audio_player_nios.c - Con VGA y 7 segmentos
#include <stdio.h>
#include <string.h>
#include "system.h"
#include <io.h>
#include <alt_types.h>
#include <unistd.h>
#include "vga.h"

// Direcciones
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920
#define SEG7_BASE         0x8800

#define AUDIO_CONTROL     0x00
#define AUDIO_FIFOSPACE   0x04
#define AUDIO_LEFT_DATA   0x08
#define AUDIO_RIGHT_DATA  0x0C

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

// Buffers para metadata
char artist_str[64];
char album_str[64];
char title_str[64];

alt_u32 sample_rate = 0;
alt_u32 num_channels = 0;
alt_u32 bits_per_sample = 0;

// Tabla 7 segmentos (ánodo común - activo bajo)
const alt_u8 seg7_table[10] = {
    0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x10
};

void display_time_7seg(alt_u32 seconds) {
    alt_u32 mins = seconds / 60;
    alt_u32 secs = seconds % 60;
    
    alt_u32 display_val = (seg7_table[mins / 10] << 24) |
                          (seg7_table[mins % 10] << 16) |
                          (seg7_table[secs / 10] << 8)  |
                          seg7_table[secs % 10];
    
    IOWR_32DIRECT(SEG7_BASE, 0, display_val);
}

void receive_string_from_fifo(char *buffer, int max_len) {
    alt_u32 len;
    int i;
    
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    len = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    if(len >= (alt_u32)max_len) len = max_len - 1;
    
    for(i = 0; i < (int)len; i++) {
        while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
        buffer[i] = (char)IORD_32DIRECT(FIFO_OUT_BASE, 0);
    }
    buffer[len] = '\0';
}

void audio_init() {
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x0C);
    usleep(1000);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_CONTROL, 0x00);
}

void wait_for_audio_space() {
    alt_u32 fifospace;
    do {
        fifospace = IORD_32DIRECT(AUDIO_BASE, AUDIO_FIFOSPACE);
    } while(((fifospace >> 24) & 0xFF) == 0 || ((fifospace >> 16) & 0xFF) == 0);
}

void send_audio_sample(alt_u32 sample) {
    alt_16 sample_signed = (alt_16)(sample & 0xFFFF);
    alt_32 sample_32 = ((alt_32)sample_signed) << 16;
    
    wait_for_audio_space();
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_LEFT_DATA, sample_32);
    IOWR_32DIRECT(AUDIO_BASE, AUDIO_RIGHT_DATA, sample_32);
}

void update_vga_time(alt_u32 seconds) {
    char tiempo_str[16];
    sprintf(tiempo_str, "%02lu:%02lu", seconds / 60, seconds % 60);
    vga_clear_line(14);
    vga_print_center(tiempo_str, 14);
}

int main() {
    alt_u32 data, status;
    alt_u32 count = 0;
    alt_u32 seconds = 0;
    
    printf("\n========================================\n");
    printf("  NIOS II Audio Player (VGA + 7seg)\n");
    printf("========================================\n\n");
    
    audio_init();
    vga_init();
    vga_clear();
    display_time_7seg(0);
    
    vga_print_center("=== AUDIO PLAYER ===", 8);
    vga_print_center("Waiting for song...", 10);
    
    printf("Waiting for ARM...\n");
    
    // Esperar METADATA_MAGIC
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            if(data == METADATA_MAGIC) break;
        }
    }
    
    // Leer metadata numérica
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
    bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);
    
    // Leer strings
    receive_string_from_fifo(artist_str, 64);
    receive_string_from_fifo(album_str, 64);
    receive_string_from_fifo(title_str, 64);
    
    // Mostrar en consola
    printf("\nNow Playing:\n");
    printf("  Title:  %s\n", title_str);
    printf("  Artist: %s\n", artist_str);
    printf("  Album:  %s\n", album_str);
    printf("  Rate:   %lu Hz\n\n", sample_rate);
    
    // Mostrar en VGA
    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 8);
    vga_print_center(title_str, 10);
    vga_print_center(artist_str, 11);
    vga_print_center(album_str, 12);
    update_vga_time(0);
    
    printf("Playing...\n");
    
    // Loop de reproducción
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            send_audio_sample(data);
            
            count++;
            
            if(count >= sample_rate) {
                seconds++;
                display_time_7seg(seconds);
                update_vga_time(seconds);
                printf("\rTime: %02lu:%02lu", seconds / 60, seconds % 60);
                count = 0;
            }
        }
    }
    
    return 0;
}