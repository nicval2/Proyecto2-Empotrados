#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// --- CONFIGURACIÓN DE DIRECCIONES ---
#define H2F_BRIDGE_BASE     0xC0000000
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900
#define FIFO_FULL  (1 << 1)
#define METADATA_MAGIC 0xDEADDA7A

// --- FUNCIONES DE MEMORIA ---
uint32_t read_u32(volatile void *addr) { return *(volatile uint32_t *)addr; }
void write_u32(volatile void *addr, uint32_t value) { *(volatile uint32_t *)addr = value; }
int fifo_has_space(volatile void *fifo_csr) { return !(read_u32(fifo_csr) & FIFO_FULL); }

// --- NUEVA FUNCIÓN: ENVIAR STRING POR FIFO ---
// Envía primero la longitud y luego letra por letra
void send_string_to_fifo(volatile void *fifo_in, volatile void *fifo_csr, const char *str) {
    uint32_t len = strlen(str);
    
    // 1. Enviar Longitud
    while(!fifo_has_space(fifo_csr));
    write_u32(fifo_in, len);
    
    // 2. Enviar Caracteres (uno por palabra de 32 bits para facilitar lectura en Nios)
    for(uint32_t i = 0; i < len; i++) {
        while(!fifo_has_space(fifo_csr));
        write_u32(fifo_in, (uint32_t)str[i]);
    }
}

// --- NUEVA FUNCIÓN: BUSCAR METADATA EN WAV ---
// Busca tags RIFF INFO: INAM (Title), IART (Artist), IPRD (Album)
void find_riff_text(FILE *f, const char *tag, char *buffer, int max_len) {
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    long current_pos = ftell(f); // Guardar posición actual
    
    // Reiniciar búsqueda desde el byte 12 (después de RIFF header)
    fseek(f, 12, SEEK_SET);

    while(fread(chunk_id, 1, 4, f) == 4) {
        fread(&chunk_size, 4, 1, f);
        chunk_id[4] = '\0';

        if(strcmp(chunk_id, "LIST") == 0) {
            char type[5] = {0};
            fread(type, 1, 4, f); // Debería ser "INFO"
            if(strncmp(type, "INFO", 4) == 0) {
                long list_end = ftell(f) + chunk_size - 4;
                
                // Buscar sub-chunks dentro de la lista INFO
                while(ftell(f) < list_end) {
                    char sub_id[5] = {0};
                    uint32_t sub_size;
                    
                    if(fread(sub_id, 1, 4, f) != 4) break;
                    fread(&sub_size, 4, 1, f);
                    sub_id[4] = '\0';
                    
                    if(strcmp(sub_id, tag) == 0) {
                        // ¡Encontrado!
                        int read_len = (sub_size < max_len) ? sub_size : max_len - 1;
                        fread(buffer, 1, read_len, f);
                        buffer[read_len] = '\0'; // Null terminator
                        
                        // Restaurar posición original del archivo
                        fseek(f, current_pos, SEEK_SET);
                        return;
                    } else {
                        fseek(f, sub_size + (sub_size % 2), SEEK_CUR); // Saltar (padding si es impar)
                    }
                }
            }
        } else {
             // Saltar chunk normal
             if (strcmp(chunk_id, "data") == 0) break; // No buscar más allá del audio
             fseek(f, chunk_size, SEEK_CUR);
        }
    }
    // Si no se encuentra, poner "Unknown"
    strcpy(buffer, "Unknown");
    fseek(f, current_pos, SEEK_SET); // Restaurar posición
}

// --- RESTO DEL CÓDIGO (Main) ---
// ... (Funciones read_chunk_header y find_data_chunk iguales al anterior) ...
int read_chunk_header(FILE *f, char *id, uint32_t *size) {
    if(fread(id, 1, 4, f) != 4) return 0;
    if(fread(size, 4, 1, f) != 1) return 0;
    return 1;
}

int find_data_chunk(FILE *f, uint32_t *data_size) {
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    while(read_chunk_header(f, chunk_id, &chunk_size)) {
        chunk_id[4] = '\0';
        if(strncmp(chunk_id, "data", 4) == 0) {
            *data_size = chunk_size;
            return 1;
        }
        fseek(f, chunk_size, SEEK_CUR);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    // ... Variables ...
    FILE *wav_file;
    int fd_mem;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    char riff[4], wave[4], fmt[4];
    uint32_t file_size, fmt_size;
    uint16_t audio_format, num_channels, bits_per_sample;
    uint32_t sample_rate, byte_rate, data_size;
    
    // Buffers para metadata
    char artist[64] = "Unknown";
    char title[64] = "Unknown";
    char album[64] = "Unknown";

    if(argc < 2) { printf("Usage: %s <wav_file>\n", argv[0]); return 1; }
    
    wav_file = fopen(argv[1], "rb");
    if(!wav_file) { perror("Open failed"); return 1; }

    // ... (Lectura de Headers estándar RIFF/WAVE/fmt igual que antes) ...
    fread(riff, 1, 4, wav_file); fread(&file_size, 4, 1, wav_file); fread(wave, 1, 4, wav_file);
    fread(fmt, 1, 4, wav_file); fread(&fmt_size, 4, 1, wav_file);
    fread(&audio_format, 2, 1, wav_file); fread(&num_channels, 2, 1, wav_file);
    fread(&sample_rate, 4, 1, wav_file); fread(&byte_rate, 4, 1, wav_file);
    fseek(wav_file, 4, SEEK_CUR); // Skip block align & bits per sample temp
    fread(&bits_per_sample, 2, 1, wav_file); // Re-read bits correctly
    
    // Volver a posición correcta si fmt > 16
    fseek(wav_file, 12 + 8 + fmt_size, SEEK_SET); 

    // --- AQUÍ BUSCAMOS LA DATA DE TEXTO ---
    printf("Extracting Metadata...\n");
    find_riff_text(wav_file, "IART", artist, 64); // Artist
    find_riff_text(wav_file, "INAM", title, 64);  // Title
    find_riff_text(wav_file, "IPRD", album, 64);  // Album (Product)
    
    printf("Artist: %s\nTitle:  %s\nAlbum:  %s\n", artist, title, album);

    // Buscar Data Chunk
    fseek(wav_file, 12, SEEK_SET); // Resetear para buscar data
    if(!find_data_chunk(wav_file, &data_size)) {
        fprintf(stderr, "Data chunk not found\n"); return 1;
    }

    // ... (Mapeo de memoria igual) ...
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;

    // --- ENVIAR CONFIGURACIÓN ---
    printf("Sending header...\n");
    while(!fifo_has_space(fifo_in_csr)); write_u32(fifo_in, METADATA_MAGIC);
    while(!fifo_has_space(fifo_in_csr)); write_u32(fifo_in, sample_rate);
    while(!fifo_has_space(fifo_in_csr)); write_u32(fifo_in, num_channels);
    while(!fifo_has_space(fifo_in_csr)); write_u32(fifo_in, bits_per_sample);
    
    // --- ENVIAR METADATA (NUEVO) ---
    printf("Sending metadata strings...\n");
    send_string_to_fifo(fifo_in, fifo_in_csr, artist);
    send_string_to_fifo(fifo_in, fifo_in_csr, album);
    send_string_to_fifo(fifo_in, fifo_in_csr, title);

    // ... (Loop de reproducción de audio igual al anterior) ...
    int16_t *audio_buffer;
    size_t buffer_samples = 1024; 
    size_t buffer_size = buffer_samples * num_channels;
    audio_buffer = (int16_t *)malloc(buffer_size * sizeof(int16_t));
    size_t samples_read;
    uint32_t total_samples_sent = 0;
    
    printf("Playing audio...\n");
    while((samples_read = fread(audio_buffer, sizeof(int16_t), buffer_size, wav_file)) > 0) {
        if(num_channels == 2) {
            for(size_t i = 0; i < samples_read; i += 2) {
                int32_t mono = (audio_buffer[i] + audio_buffer[i+1]) / 2;
                while(!fifo_has_space(fifo_in_csr));
                write_u32(fifo_in, (uint32_t)(mono & 0xFFFF));
                total_samples_sent++;
            }
        } else {
             for(size_t i = 0; i < samples_read; i++) {
                while(!fifo_has_space(fifo_in_csr));
                write_u32(fifo_in, (uint32_t)(audio_buffer[i] & 0xFFFF));
                total_samples_sent++;
            }
        }
    }
    
    // Cleanup
    free(audio_buffer);
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    fclose(wav_file);
    return 0;
}