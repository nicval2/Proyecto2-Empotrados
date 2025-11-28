// wav_player_arm_stereo_fixed.c
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// Configuración H2F
#define H2F_BRIDGE_BASE     0xC0000000
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

#define FIFO_FULL  (1 << 1)
#define METADATA_MAGIC 0xDEADDA7A

// Funciones de acceso a memoria
uint32_t read_u32(volatile void *addr) {
    uint32_t value;
    memcpy(&value, (void*)addr, sizeof(uint32_t));
    return value;
}

void write_u32(volatile void *addr, uint32_t value) {
    memcpy((void*)addr, &value, sizeof(uint32_t));
}

int fifo_has_space(volatile void *fifo_csr) {
    uint32_t status = read_u32(fifo_csr);
    return !(status & FIFO_FULL);
}

// Leer chunk del WAV
int read_chunk_header(FILE *f, char *id, uint32_t *size) {
    if(fread(id, 1, 4, f) != 4) return 0;
    if(fread(size, 4, 1, f) != 1) return 0;
    return 1;
}

// Buscar chunk "data"
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
    FILE *wav_file;
    int fd_mem;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    
    char riff[4], wave[4], fmt[4];
    uint32_t file_size, fmt_size;
    uint16_t audio_format, num_channels, bits_per_sample, block_align;
    uint32_t sample_rate, byte_rate, data_size;
    
    int16_t *audio_buffer;
    size_t buffer_samples = 2048;  // Muestras por canal
    size_t samples_read;
    uint32_t total_samples_sent = 0;
    size_t i;
    
    if(argc < 2) {
        printf("Usage: %s <wav_file>\n", argv[0]);
        return 1;
    }
    
    printf("========================================\n");
    printf("WAV Player - ARM to NIOS (Stereo Fixed)\n");
    printf("========================================\n\n");
    
    // Abrir archivo
    wav_file = fopen(argv[1], "rb");
    if(!wav_file) {
        perror("Error opening file");
        return 1;
    }
    
    printf("[OK] Opened: %s\n", argv[1]);
    
    // Leer RIFF header
    fread(riff, 1, 4, wav_file);
    fread(&file_size, 4, 1, wav_file);
    fread(wave, 1, 4, wav_file);
    
    if(strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        fprintf(stderr, "Not a valid WAV file\n");
        fclose(wav_file);
        return 1;
    }
    
    // Leer fmt chunk
    fread(fmt, 1, 4, wav_file);
    fread(&fmt_size, 4, 1, wav_file);
    
    if(strncmp(fmt, "fmt ", 4) != 0) {
        fprintf(stderr, "Invalid fmt chunk\n");
        fclose(wav_file);
        return 1;
    }
    
    fread(&audio_format, 2, 1, wav_file);
    fread(&num_channels, 2, 1, wav_file);
    fread(&sample_rate, 4, 1, wav_file);
    fread(&byte_rate, 4, 1, wav_file);
    fread(&block_align, 2, 1, wav_file);
    fread(&bits_per_sample, 2, 1, wav_file);
    
    if(fmt_size > 16) {
        fseek(wav_file, fmt_size - 16, SEEK_CUR);
    }
    
    // Buscar chunk "data"
    if(!find_data_chunk(wav_file, &data_size)) {
        fprintf(stderr, "Data chunk not found\n");
        fclose(wav_file);
        return 1;
    }
    
    // Mostrar información
    printf("\nWAV Information:\n");
    printf("  Sample Rate:    %u Hz\n", sample_rate);
    printf("  Channels:       %u (%s)\n", num_channels,
           num_channels == 1 ? "Mono" : "Stereo");
    printf("  Bits/Sample:    %u bits\n", bits_per_sample);
    printf("  Data Size:      %u bytes (%.2f MB)\n", 
           data_size, data_size / 1048576.0f);
    
    uint32_t total_samples = data_size / (bits_per_sample / 8);
    float duration = (float)total_samples / sample_rate / num_channels;
    printf("  Duration:       %.2f seconds\n\n", duration);
    
    if(audio_format != 1 || bits_per_sample != 16) {
        fprintf(stderr, "Only 16-bit PCM supported\n");
        fclose(wav_file);
        return 1;
    }
    
    // Abrir /dev/mem
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        perror("Error opening /dev/mem");
        fclose(wav_file);
        return 1;
    }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, 
                   MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    
    if(h2f_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd_mem);
        fclose(wav_file);
        return 1;
    }
    
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;
    
    printf("[OK] Memory mapped\n");
    
    // Enviar metadata
    printf("Sending metadata...\n");
    
    while(!fifo_has_space(fifo_in_csr)) usleep(1000);
    write_u32(fifo_in, METADATA_MAGIC);
    
    while(!fifo_has_space(fifo_in_csr)) usleep(1000);
    write_u32(fifo_in, sample_rate);
    
    while(!fifo_has_space(fifo_in_csr)) usleep(1000);
    write_u32(fifo_in, num_channels);
    
    while(!fifo_has_space(fifo_in_csr)) usleep(1000);
    write_u32(fifo_in, bits_per_sample);
    
    printf("[OK] Metadata sent\n\n");
    
    // Alocar buffer (stereo = 2 canales)
    size_t buffer_size = buffer_samples * num_channels;
    audio_buffer = (int16_t *)malloc(buffer_size * sizeof(int16_t));
    if(!audio_buffer) {
        fprintf(stderr, "Error allocating buffer\n");
        munmap(h2f_map, 0x10000);
        close(fd_mem);
        fclose(wav_file);
        return 1;
    }
    
    printf("Starting playback...\n");
    printf("Press Ctrl+C to stop\n");
    printf("-----------------------------------\n");
    
    uint32_t progress_marker = 0;
    
    // Leer y enviar audio
    while((samples_read = fread(audio_buffer, sizeof(int16_t), 
                                buffer_size, wav_file)) > 0) {
        
        if(num_channels == 2) {
            // Stereo: Promediar L y R para enviar mono
            for(i = 0; i < samples_read; i += 2) {
                // Promediar canales L y R
                int32_t left = audio_buffer[i];
                int32_t right = audio_buffer[i + 1];
                int32_t mono = (left + right) / 2;
                
                uint32_t sample = (uint32_t)(mono & 0xFFFF);
                
                while(!fifo_has_space(fifo_in_csr)) {
                    usleep(50);
                }
                
                write_u32(fifo_in, sample);
                total_samples_sent++;
            }
        } else {
            // Mono
            for(i = 0; i < samples_read; i++) {
                uint32_t sample = (uint32_t)(audio_buffer[i] & 0xFFFF);
                
                while(!fifo_has_space(fifo_in_csr)) {
                    usleep(50);
                }
                
                write_u32(fifo_in, sample);
                total_samples_sent++;
            }
        }
        
        // Progreso cada 5%
        uint32_t current_progress = (total_samples_sent * 100 * num_channels) / 
                                    (total_samples);
        if(current_progress >= progress_marker + 5) {
            progress_marker = current_progress;
            float elapsed = (float)total_samples_sent / sample_rate;
            printf("Progress: %u%% (%.1f / %.1f sec)\n", 
                   current_progress, elapsed, duration);
        }
    }
    
    printf("\n-----------------------------------\n");
    printf("[OK] Playback complete!\n");
    
    free(audio_buffer);
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    fclose(wav_file);
    
    return 0;
}