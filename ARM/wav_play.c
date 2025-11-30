#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>  // <--- IMPORTANTE: Necesario para ver el error real

/* --- DIRECCIONES DE HARDWARE --- */
#define H2F_BRIDGE_BASE     0xC0000000
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

#define FIFO_FULL           (1 << 1)

/* --- TOKENS DE PROTOCOLO --- */
#define METADATA_MAGIC      0xDEADDA7A
#define EOS_TOKEN           0xFFFFFFFF  // Token de Fin de Cancion

typedef struct {
    char title[64];
    char artist[64];
    char album[64];
} WavMetadata;

/* --- FUNCIONES DE ACCESO A MEMORIA --- */
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
    
    // Luego cada caracter
    for(i = 0; i < len; i++) {
        fifo_write_wait(fifo_in, fifo_csr, (uint32_t)(unsigned char)str[i]);
    }
}

/* --- FUNCIONES DE PARSEO WAV --- */
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
    
    // Valores por defecto
    strcpy(meta->title, "Desconocido");
    strcpy(meta->artist, "Desconocido");
    strcpy(meta->album, "Desconocido");
    
    rewind(f); // Asegurar inicio
    char riff[4], wave[4];
    uint32_t file_size;
    fread(riff, 1, 4, f);
    fread(&file_size, 4, 1, f);
    fread(wave, 1, 4, f);
    
    if(strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) return 0;

    while(read_chunk_header(f, chunk_id, &chunk_size)) {
        chunk_id[4] = '\0';
        
        if(strncmp(chunk_id, "fmt ", 4) == 0) {
            fmt_start = ftell(f);
            fread(audio_format, 2, 1, f);
            fread(num_channels, 2, 1, f);
            fread(sample_rate, 4, 1, f);
            fseek(f, 4, SEEK_CUR); // Byte rate
            fseek(f, 2, SEEK_CUR); // Block align
            fread(bits_per_sample, 2, 1, f);
            fseek(f, fmt_start + chunk_size, SEEK_SET);
        }
        else if(strncmp(chunk_id, "LIST", 4) == 0) {
            parse_list_chunk(f, chunk_size, meta);
        }
        else if(strncmp(chunk_id, "data", 4) == 0) {
            *data_size = chunk_size;
            return 1; // Encontramos datos, éxito
        }
        else {
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
        }
    }
    
    return 0;
}

/* --- FUNCIÓN PARA REPRODUCIR UN ARCHIVO --- */
void play_file(const char* filename, volatile void *fifo_in, volatile void *fifo_csr) {
    FILE *wav_file;
    uint32_t data_size;
    uint16_t audio_format, num_channels, bits_per_sample;
    uint32_t sample_rate;
    WavMetadata metadata;
    
    int16_t *audio_buffer;
    size_t buffer_samples = 2048;
    size_t samples_read;
    size_t i;
    
    printf("\nAbriendo: %s\n", filename);
    wav_file = fopen(filename, "rb");
    if(!wav_file) {
        printf("Error: No se pudo abrir %s\n", filename);
        return;
    }
    
    if(!parse_wav_file(wav_file, &data_size, &metadata, 
                       &audio_format, &num_channels, &sample_rate, &bits_per_sample)) {
        printf("Error: No se encontro chunk 'data' en %s\n", filename);
        fclose(wav_file);
        return;
    }
    
    // --- CORRECCIÓN AQUÍ: ACEPTAR FORMATO 1 (PCM) Y 65534 (EXTENSIBLE) ---
    if(audio_format != 1 && audio_format != 65534) {
        printf("Error: Formato %d no soportado. Solo PCM (1) o Extensible (65534).\n", audio_format);
        fclose(wav_file);
        return;
    }
    
    if(bits_per_sample != 16) {
        printf("Error: Solo 16 bits soportado (archivo es %d)\n", bits_per_sample);
        fclose(wav_file);
        return;
    }
    
    // Info
    printf("  Titulo:  %s\n", metadata.title);
    printf("  Artista: %s\n", metadata.artist);
    printf("  Rate:    %u Hz, %d ch\n", sample_rate, num_channels);

    // Enviar Metadata al FPGA
    fifo_write_wait(fifo_in, fifo_csr, METADATA_MAGIC);
    fifo_write_wait(fifo_in, fifo_csr, sample_rate);
    fifo_write_wait(fifo_in, fifo_csr, num_channels);
    fifo_write_wait(fifo_in, fifo_csr, bits_per_sample);
    
    fifo_write_string(fifo_in, fifo_csr, metadata.artist);
    fifo_write_string(fifo_in, fifo_csr, metadata.album);
    fifo_write_string(fifo_in, fifo_csr, metadata.title);
    
    printf("  --> Reproduciendo...\n");
    
    // Buffer
    audio_buffer = (int16_t *)malloc(buffer_samples * num_channels * sizeof(int16_t));
    if(!audio_buffer) {
        printf("Error malloc\n");
        fclose(wav_file);
        return;
    }
    
    // Enviar Audio
    while((samples_read = fread(audio_buffer, sizeof(int16_t), buffer_samples * num_channels, wav_file)) > 0) {
        if(num_channels == 2) {
            for(i = 0; i < samples_read; i += 2) {
                // Mezcla simple Stereo -> Mono
                int32_t left = audio_buffer[i];
                int32_t right = audio_buffer[i + 1];
                int32_t mono = (left + right) / 2;
                
                fifo_write_wait(fifo_in, fifo_csr, (uint32_t)(mono & 0xFFFF));
            }
        } else {
            // Mono directo
            for(i = 0; i < samples_read; i++) {
                fifo_write_wait(fifo_in, fifo_csr, (uint32_t)(audio_buffer[i] & 0xFFFF));
            }
        }
    }
    
    // --- ENVIAR SEÑAL DE FIN DE CANCION ---
    printf("  --> Fin archivo. Enviando EOS.\n");
    fifo_write_wait(fifo_in, fifo_csr, EOS_TOKEN);
    
    free(audio_buffer);
    fclose(wav_file);
}

/* --- MAIN --- */
int main(int argc, char *argv[]) {
    int fd_mem;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    
    if(argc < 2) {
        printf("Uso: %s <cancion1.wav> [cancion2.wav ...]\n", argv[0]);
        return 1;
    }
    
    // Mapeo de memoria
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        printf("Error fatal: open('/dev/mem') fallo.\n");
        printf("Errno: %d (%s)\n", errno, strerror(errno));
        return 1;
    }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    if(h2f_map == MAP_FAILED) {
        printf("Error fatal: mmap() fallo.\n");
        printf("Errno: %d (%s)\n", errno, strerror(errno));
        close(fd_mem);
        return 1;
    }
    
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;
    
    printf("=== REPRODUCTOR HPS (Multiformato + Playlist) ===\n");
    
    // Bucle Playlist
    for(int i = 1; i < argc; i++) {
        printf("\n[ Pista %d de %d ]", i, argc - 1);
        play_file(argv[i], fifo_in, fifo_in_csr);
        // Pequeña pausa entre canciones
        usleep(500000); 
    }
    
    printf("\n=== Playlist finalizada ===\n");
    
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    
    return 0;
}