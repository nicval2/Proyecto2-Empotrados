// wav_player_arm_stereo_fixed_v2.c
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// Configuración H2F (Verifica tus offsets si son distintos)
#define H2F_BRIDGE_BASE     0xC0000000
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

#define FIFO_FULL  (1 << 1)
#define METADATA_MAGIC 0xDEADDA7A

// --- Funciones de Memoria ---
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

// --- Parsers WAV ---
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
    FILE *wav_file;
    int fd_mem;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    
    char riff[4], wave[4], fmt[4];
    uint32_t file_size, fmt_size, data_size;
    uint16_t audio_format, num_channels, bits_per_sample, block_align;
    uint32_t sample_rate, byte_rate;
    
    int16_t *audio_buffer;
    // AUMENTAMOS BUFFER A 8192 PARA EVITAR EL "TREN"
    size_t buffer_samples = 8192; 
    size_t samples_read, i;
    uint32_t total_samples_sent = 0;
    
    if(argc < 2) {
        printf("Uso: %s <archivo.wav>\n", argv[0]);
        return 1;
    }
    
    // --- 1. ABRIR Y LEER HEADERS ---
    wav_file = fopen(argv[1], "rb");
    if(!wav_file) { perror("Error archivo"); return 1; }
    
    fread(riff, 1, 4, wav_file);
    fread(&file_size, 4, 1, wav_file);
    fread(wave, 1, 4, wav_file);
    fread(fmt, 1, 4, wav_file);
    fread(&fmt_size, 4, 1, wav_file);
    fread(&audio_format, 2, 1, wav_file);
    fread(&num_channels, 2, 1, wav_file);
    fread(&sample_rate, 4, 1, wav_file);
    fread(&byte_rate, 4, 1, wav_file);
    fread(&block_align, 2, 1, wav_file);
    fread(&bits_per_sample, 2, 1, wav_file);
    
    if(fmt_size > 16) fseek(wav_file, fmt_size - 16, SEEK_CUR);
    if(!find_data_chunk(wav_file, &data_size)) { printf("No data chunk\n"); return 1; }

    printf("WAV Info: %u Hz, %u Canales, %u Bits\n", sample_rate, num_channels, bits_per_sample);

    // --- 2. MAPEAR MEMORIA ---
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) { perror("Error /dev/mem"); return 1; }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    if(h2f_map == MAP_FAILED) { perror("mmap failed"); return 1; }
    
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;

    // --- 3. ENVIAR METADATA (HANDSHAKE) ---
    printf("Enviando Metadata...\n");
    while(!fifo_has_space(fifo_in_csr)) usleep(100); write_u32(fifo_in, METADATA_MAGIC);
    while(!fifo_has_space(fifo_in_csr)) usleep(100); write_u32(fifo_in, sample_rate);
    while(!fifo_has_space(fifo_in_csr)) usleep(100); write_u32(fifo_in, num_channels);
    while(!fifo_has_space(fifo_in_csr)) usleep(100); write_u32(fifo_in, bits_per_sample);
    
    // --- 4. BUFFER Y REPRODUCCIÓN ---
    size_t buffer_size_bytes = buffer_samples * num_channels * sizeof(int16_t);
    audio_buffer = (int16_t *)malloc(buffer_size_bytes);
    
    printf("Reproduciendo... (Ctrl+C para salir)\n");

    while((samples_read = fread(audio_buffer, sizeof(int16_t), buffer_samples * num_channels, wav_file)) > 0) {
        
        if(num_channels == 2) {
            // === MODO ESTEREO REAL ===
            // Leemos 2 muestras (L y R) y enviamos 2 muestras a la FIFO
            for(i = 0; i < samples_read; i += 2) {
                
                int16_t left_val = audio_buffer[i];
                int16_t right_val = audio_buffer[i+1];

                // Enviar L
                while(!fifo_has_space(fifo_in_csr)); // Busy wait rapido
                write_u32(fifo_in, (uint32_t)(left_val & 0xFFFF));

                // Enviar R
                while(!fifo_has_space(fifo_in_csr)); 
                write_u32(fifo_in, (uint32_t)(right_val & 0xFFFF));
            }
        } 
        else {
            // === MODO MONO ===
            for(i = 0; i < samples_read; i++) {
                int16_t val = audio_buffer[i];
                
                while(!fifo_has_space(fifo_in_csr)); 
                write_u32(fifo_in, (uint32_t)(val & 0xFFFF));
            }
        }
    }
    
    printf("\nFin de cancion.\n");
    free(audio_buffer);
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    fclose(wav_file);
    return 0;
}