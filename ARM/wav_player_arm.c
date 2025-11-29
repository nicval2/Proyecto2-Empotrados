// wav_player_arm.c - Con metadata de texto
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define H2F_BRIDGE_BASE     0xC0000000
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

#define FIFO_FULL  (1 << 1)
#define METADATA_MAGIC 0xDEADDA7A

typedef struct {
    char title[64];
    char artist[64];
    char album[64];
} WavMetadata;

uint32_t read_u32(volatile void *addr) {
    return *(volatile uint32_t *)addr;
}

void write_u32(volatile void *addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

int fifo_has_space(volatile void *fifo_csr) {
    return !(read_u32(fifo_csr) & FIFO_FULL);
}

void fifo_write_wait(volatile void *fifo_in, volatile void *fifo_csr, uint32_t value) {
    while(!fifo_has_space(fifo_csr));
    write_u32(fifo_in, value);
}

// Enviar string caracter por caracter
void fifo_write_string(volatile void *fifo_in, volatile void *fifo_csr, const char *str) {
    size_t len = strlen(str);
    size_t i;
    
    if(len > 63) len = 63;
    
    // Primero enviar longitud
    fifo_write_wait(fifo_in, fifo_csr, len);
    
    // Luego cada caracter como un valor de 32 bits
    for(i = 0; i < len; i++) {
        fifo_write_wait(fifo_in, fifo_csr, (uint32_t)(unsigned char)str[i]);
    }
}

int read_chunk_header(FILE *f, char *id, uint32_t *size) {
    if(fread(id, 1, 4, f) != 4) return 0;
    if(fread(size, 4, 1, f) != 1) return 0;
    return 1;
}

void read_info_string(FILE *f, uint32_t size, char *dest, size_t dest_size) {
    size_t to_read = (size < dest_size - 1) ? size : dest_size - 1;
    fread(dest, 1, to_read, f);
    dest[to_read] = '\0';
    
    if(size > to_read) fseek(f, size - to_read, SEEK_CUR);
    if(size % 2 != 0) fseek(f, 1, SEEK_CUR);
}

void parse_list_chunk(FILE *f, uint32_t list_size, WavMetadata *meta) {
    char list_type[4];
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    uint32_t bytes_read;
    
    fread(list_type, 1, 4, f);
    
    if(strncmp(list_type, "INFO", 4) != 0) {
        fseek(f, list_size - 4, SEEK_CUR);
        return;
    }
    
    bytes_read = 4;
    
    while(bytes_read < list_size) {
        if(!read_chunk_header(f, chunk_id, &chunk_size)) break;
        chunk_id[4] = '\0';
        bytes_read += 8;
        
        if(strncmp(chunk_id, "IART", 4) == 0) {
            read_info_string(f, chunk_size, meta->artist, sizeof(meta->artist));
        } else if(strncmp(chunk_id, "INAM", 4) == 0) {
            read_info_string(f, chunk_size, meta->title, sizeof(meta->title));
        } else if(strncmp(chunk_id, "IPRD", 4) == 0) {
            read_info_string(f, chunk_size, meta->album, sizeof(meta->album));
        } else {
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
        }
        
        bytes_read += chunk_size + (chunk_size % 2);
    }
}

int parse_wav_file(FILE *f, uint32_t *data_size, WavMetadata *meta, 
                   uint16_t *audio_format, uint16_t *num_channels,
                   uint32_t *sample_rate, uint16_t *bits_per_sample) {
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    long fmt_start;
    
    strcpy(meta->title, "Unknown");
    strcpy(meta->artist, "Unknown");
    strcpy(meta->album, "Unknown");
    
    while(read_chunk_header(f, chunk_id, &chunk_size)) {
        chunk_id[4] = '\0';
        
        if(strncmp(chunk_id, "fmt ", 4) == 0) {
            fmt_start = ftell(f);
            fread(audio_format, 2, 1, f);
            fread(num_channels, 2, 1, f);
            fread(sample_rate, 4, 1, f);
            fseek(f, 4, SEEK_CUR);
            fseek(f, 2, SEEK_CUR);
            fread(bits_per_sample, 2, 1, f);
            fseek(f, fmt_start + chunk_size, SEEK_SET);
        }
        else if(strncmp(chunk_id, "LIST", 4) == 0) {
            parse_list_chunk(f, chunk_size, meta);
        }
        else if(strncmp(chunk_id, "data", 4) == 0) {
            *data_size = chunk_size;
            return 1;
        }
        else {
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
        }
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    FILE *wav_file;
    int fd_mem;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    
    char riff[4], wave[4];
    uint32_t file_size, data_size;
    uint16_t audio_format, num_channels, bits_per_sample;
    uint32_t sample_rate;
    WavMetadata metadata;
    
    int16_t *audio_buffer;
    size_t buffer_samples = 2048;
    size_t samples_read;
    uint32_t total_samples_sent = 0;
    size_t i;
    size_t buffer_size;
    uint32_t total_samples;
    float duration;
    
    if(argc < 2) {
        printf("Usage: %s <wav_file>\n", argv[0]);
        return 1;
    }
    
    printf("========================================\n");
    printf("WAV Player ARM (with Metadata)\n");
    printf("========================================\n\n");
    
    wav_file = fopen(argv[1], "rb");
    if(!wav_file) {
        perror("Error opening file");
        return 1;
    }
    
    fread(riff, 1, 4, wav_file);
    fread(&file_size, 4, 1, wav_file);
    fread(wave, 1, 4, wav_file);
    
    if(strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        fprintf(stderr, "Not a valid WAV file\n");
        fclose(wav_file);
        return 1;
    }
    
    if(!parse_wav_file(wav_file, &data_size, &metadata, 
                       &audio_format, &num_channels, &sample_rate, &bits_per_sample)) {
        fprintf(stderr, "Data chunk not found\n");
        fclose(wav_file);
        return 1;
    }
    
    total_samples = data_size / (bits_per_sample / 8);
    duration = (float)total_samples / sample_rate / num_channels;
    
    printf("Title:       %s\n", metadata.title);
    printf("Artist:      %s\n", metadata.artist);
    printf("Album:       %s\n", metadata.album);
    printf("Sample Rate: %u Hz\n", sample_rate);
    printf("Duration:    %.1f sec\n\n", duration);
    
    if(bits_per_sample != 16) {
        fprintf(stderr, "Only 16-bit PCM supported\n");
        fclose(wav_file);
        return 1;
    }
    
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        perror("Error opening /dev/mem");
        fclose(wav_file);
        return 1;
    }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    if(h2f_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd_mem);
        fclose(wav_file);
        return 1;
    }
    
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;
    
    printf("Sending metadata...\n");
    
    // Enviar metadata numérica
    fifo_write_wait(fifo_in, fifo_in_csr, METADATA_MAGIC);
    fifo_write_wait(fifo_in, fifo_in_csr, sample_rate);
    fifo_write_wait(fifo_in, fifo_in_csr, num_channels);
    fifo_write_wait(fifo_in, fifo_in_csr, bits_per_sample);
    
    // Enviar strings
    fifo_write_string(fifo_in, fifo_in_csr, metadata.artist);
    fifo_write_string(fifo_in, fifo_in_csr, metadata.album);
    fifo_write_string(fifo_in, fifo_in_csr, metadata.title);
    
    printf("Playing...\n");
    
    buffer_size = buffer_samples * num_channels;
    audio_buffer = (int16_t *)malloc(buffer_size * sizeof(int16_t));
    
    while((samples_read = fread(audio_buffer, sizeof(int16_t), buffer_size, wav_file)) > 0) {
        
        if(num_channels == 2) {
            for(i = 0; i < samples_read; i += 2) {
                int32_t left = audio_buffer[i];
                int32_t right = audio_buffer[i + 1];
                int32_t mono = (left + right) / 2;
                
                uint32_t sample = (uint32_t)(mono & 0xFFFF);
                
                while(!fifo_has_space(fifo_in_csr));
                write_u32(fifo_in, sample);
                total_samples_sent++;
            }
        } else {
            for(i = 0; i < samples_read; i++) {
                uint32_t sample = (uint32_t)(audio_buffer[i] & 0xFFFF);
                
                while(!fifo_has_space(fifo_in_csr));
                write_u32(fifo_in, sample);
                total_samples_sent++;
            }
        }
    }
    
    printf("Playback complete.\n");
    
    free(audio_buffer);
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    fclose(wav_file);
    
    return 0;
}