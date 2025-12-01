// wav_player_arm.c - HPS con soporte Next/Prev (CORREGIDO)
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

/* --- DIRECCIONES DE HARDWARE --- */
#define H2F_BRIDGE_BASE     0xC0000000

// FIFO: HPS -> NIOS (escritura de audio)
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

// FIFO2: NIOS -> HPS (lectura de comandos)
#define FIFO2_OUT_OFFSET     0x8960
#define FIFO2_OUT_CSR_OFFSET 0x8980

/* --- FLAGS DE FIFO --- */
#define FIFO_FULL   (1 << 1)
#define FIFO_EMPTY  (1 << 0)

/* --- TOKENS DE PROTOCOLO --- */
#define METADATA_MAGIC  0xDEADDA7A
#define EOS_TOKEN       0xFFFFFFFF
#define SKIP_TOKEN      0xFFFFFFFE

/* --- COMANDOS NIOS -> HPS --- */
#define CMD_NEXT  0x4E455854
#define CMD_PREV  0x50524556

typedef struct {
    char title[64];
    char artist[64];
    char album[64];
} WavMetadata;

/* --- PUNTEROS GLOBALES A HARDWARE --- */
static volatile uint32_t *fifo_in;
static volatile uint32_t *fifo_in_csr;
static volatile uint32_t *fifo2_out;
static volatile uint32_t *fifo2_out_csr;

/* --- VARIABLES DE ESTADO --- */
static int current_track = 0;
static int total_tracks = 0;
static int skip_requested = 0;

/* --- FUNCIONES DE ACCESO A HARDWARE --- */

int fifo_has_space(void) {
    return !(*fifo_in_csr & FIFO_FULL);
}

int fifo2_has_data(void) {
    return !(*fifo2_out_csr & FIFO_EMPTY);
}

void fifo_write_wait(uint32_t value) {
    while(!fifo_has_space());
    *fifo_in = value;
}

void fifo_write_string(const char *str) {
    size_t len = strlen(str);
    if(len > 63) len = 63;
    
    fifo_write_wait(len);
    for(size_t i = 0; i < len; i++) {
        fifo_write_wait((uint32_t)(unsigned char)str[i]);
    }
}

/* Verificar comandos del NIOS (con límite de seguridad) */
void check_commands(void) {
    int count = 0;
    while(fifo2_has_data() && count < 5) {
        uint32_t cmd = *fifo2_out;
        count++;
        
        if(cmd == CMD_NEXT) {
            printf("[CMD] NEXT\n"); fflush(stdout);
            skip_requested = 1;
        }
        else if(cmd == CMD_PREV) {
            printf("[CMD] PREV\n"); fflush(stdout);
            skip_requested = -1;
        }
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
    uint32_t chunk_size, bytes_read;
    
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
        
        if(strncmp(chunk_id, "IART", 4) == 0)
            read_info_string(f, chunk_size, meta->artist, sizeof(meta->artist));
        else if(strncmp(chunk_id, "INAM", 4) == 0)
            read_info_string(f, chunk_size, meta->title, sizeof(meta->title));
        else if(strncmp(chunk_id, "IPRD", 4) == 0)
            read_info_string(f, chunk_size, meta->album, sizeof(meta->album));
        else
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
        
        bytes_read += chunk_size + (chunk_size % 2);
    }
}

int parse_wav_file(FILE *f, uint32_t *data_size, WavMetadata *meta, 
                   uint16_t *audio_format, uint16_t *num_channels,
                   uint32_t *sample_rate, uint16_t *bits_per_sample) {
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    long fmt_start;
    
    strcpy(meta->title, "Desconocido");
    strcpy(meta->artist, "Desconocido");
    strcpy(meta->album, "Desconocido");
    
    rewind(f);
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
            fseek(f, 6, SEEK_CUR);
            fread(bits_per_sample, 2, 1, f);
            fseek(f, fmt_start + chunk_size, SEEK_SET);
        }
        else if(strncmp(chunk_id, "LIST", 4) == 0)
            parse_list_chunk(f, chunk_size, meta);
        else if(strncmp(chunk_id, "data", 4) == 0) {
            *data_size = chunk_size;
            return 1;
        }
        else
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
    }
    return 0;
}

/* --- REPRODUCIR ARCHIVO --- */
int play_file(const char* filename) {
    FILE *wav_file;
    uint32_t data_size;
    uint16_t audio_format, num_channels, bits_per_sample;
    uint32_t sample_rate;
    WavMetadata metadata;
    int16_t *audio_buffer;
    size_t buffer_samples = 2048;
    size_t samples_read;
    int result = 0;
    int check_counter = 0;
    
    skip_requested = 0;
    
    printf("\n[%d/%d] %s\n", current_track + 1, total_tracks, filename);
    fflush(stdout);
    
    wav_file = fopen(filename, "rb");
    if(!wav_file) {
        printf("Error: No se pudo abrir\n");
        return 0;
    }
    
    if(!parse_wav_file(wav_file, &data_size, &metadata, 
                       &audio_format, &num_channels, &sample_rate, &bits_per_sample)) {
        printf("Error: WAV invalido\n");
        fclose(wav_file);
        return 0;
    }
    
    if((audio_format != 1 && audio_format != 65534) || bits_per_sample != 16) {
        printf("Error: Formato no soportado\n");
        fclose(wav_file);
        return 0;
    }
    
    printf("  %s - %s (%u Hz)\n", metadata.artist, metadata.title, sample_rate);
    fflush(stdout);

    // Enviar Metadata
    fifo_write_wait(METADATA_MAGIC);
    fifo_write_wait(sample_rate);
    fifo_write_wait(num_channels);
    fifo_write_wait(bits_per_sample);
    fifo_write_string(metadata.artist);
    fifo_write_string(metadata.album);
    fifo_write_string(metadata.title);
    
    printf("  Reproduciendo...\n");
    fflush(stdout);
    
    audio_buffer = (int16_t *)malloc(buffer_samples * num_channels * sizeof(int16_t));
    if(!audio_buffer) {
        fclose(wav_file);
        return 0;
    }
    
    while((samples_read = fread(audio_buffer, sizeof(int16_t), 
                                 buffer_samples * num_channels, wav_file)) > 0) {
        
        // Verificar comandos periódicamente
        if(++check_counter >= 50) {
            check_commands();
            check_counter = 0;
            
            if(skip_requested != 0) {
                printf("  Skip!\n"); fflush(stdout);
                result = skip_requested;
                break;
            }
        }
        
        // Enviar audio
        if(num_channels == 2) {
            for(size_t i = 0; i < samples_read; i += 2) {
                int32_t mono = ((int32_t)audio_buffer[i] + audio_buffer[i+1]) / 2;
                fifo_write_wait((uint32_t)(mono & 0xFFFF));
            }
        } else {
            for(size_t i = 0; i < samples_read; i++) {
                fifo_write_wait((uint32_t)(audio_buffer[i] & 0xFFFF));
            }
        }
    }
    
    // Enviar token de fin
    fifo_write_wait(result ? SKIP_TOKEN : EOS_TOKEN);
    printf("  %s\n", result ? ">> SKIP" : ">> FIN");
    fflush(stdout);
    
    free(audio_buffer);
    fclose(wav_file);
    return result;
}

/* --- MAIN --- */
int main(int argc, char *argv[]) {
    int fd_mem;
    void *h2f_map;
    
    // Deshabilitar buffering de stdout
    setbuf(stdout, NULL);
    
    if(argc < 2) {
        printf("Uso: %s <archivo.wav> [...]\n", argv[0]);
        return 1;
    }
    
    total_tracks = argc - 1;
    
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        printf("Error: /dev/mem - %s\n", strerror(errno));
        return 1;
    }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    if(h2f_map == MAP_FAILED) {
        printf("Error: mmap - %s\n", strerror(errno));
        close(fd_mem);
        return 1;
    }
    
    // Configurar punteros
    fifo_in = (volatile uint32_t *)(h2f_map + FIFO_IN_OFFSET);
    fifo_in_csr = (volatile uint32_t *)(h2f_map + FIFO_IN_CSR_OFFSET);
    fifo2_out = (volatile uint32_t *)(h2f_map + FIFO2_OUT_OFFSET);
    fifo2_out_csr = (volatile uint32_t *)(h2f_map + FIFO2_OUT_CSR_OFFSET);
    
    printf("=== REPRODUCTOR HPS v2.1 ===\n");
    printf("Pistas: %d\n", total_tracks);
    printf("KEY2=Next, KEY1=Prev, KEY3=Pause\n\n");
    
    // DEBUG: Mostrar estado inicial de FIFOs
    printf("[DEBUG] FIFO_IN_CSR = 0x%08X\n", *fifo_in_csr);
    printf("[DEBUG] FIFO2_OUT_CSR = 0x%08X\n", *fifo2_out_csr);
    
    // Bucle de playlist
    current_track = 0;
    while(1) {
        int result = play_file(argv[current_track + 1]);
        
        if(result == 1) {
            current_track = (current_track + 1) % total_tracks;
        }
        else if(result == -1) {
            current_track = (current_track - 1 + total_tracks) % total_tracks;
        }
        else {
            current_track++;
            if(current_track >= total_tracks) {
                printf("\n=== Reiniciando playlist ===\n");
                current_track = 0;
            }
        }
        usleep(300000);
    }
    
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    return 0;
}