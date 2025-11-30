// wav_player_playlist.c
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Direcciones H2F Bridge
#define H2F_BRIDGE_BASE     0xC0000000

// FIFO (ARM -> NIOS)
#define FIFO_IN_OFFSET      0x8860
#define FIFO_IN_CSR_OFFSET  0x8900

// FIFO2 (NIOS -> ARM)
#define FIFO2_OUT_OFFSET     0x8950
#define FIFO2_OUT_CSR_OFFSET 0x8960

#define FIFO_FULL  (1 << 1)
#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

#define CMD_NEXT 1
#define CMD_PREV 2

#define MAX_SONGS 50
#define MAX_PATH 256

typedef struct {
    char title[64];
    char artist[64];
    char album[64];
} WavMetadata;

typedef struct {
    char filepath[MAX_PATH];
    WavMetadata meta;
    uint32_t sample_rate;
    uint16_t num_channels;
    uint16_t bits_per_sample;
    uint32_t data_size;
    long data_offset;
} Song;

Song playlist[MAX_SONGS];
int playlist_count = 0;
int current_song = 0;
volatile int skip_requested = 0;
volatile int skip_direction = 0;

void *h2f_map = NULL;
volatile void *fifo_in = NULL;
volatile void *fifo_in_csr = NULL;
volatile void *fifo2_out = NULL;
volatile void *fifo2_out_csr = NULL;

// ============ Funciones de acceso a memoria ============

uint32_t read_u32(volatile void *addr) {
    return *(volatile uint32_t *)addr;
}

void write_u32(volatile void *addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
}

int fifo_has_space(volatile void *csr) {
    return !(read_u32(csr) & FIFO_FULL);
}

int fifo2_has_data(volatile void *csr) {
    return !(read_u32(csr) & FIFO_EMPTY);
}

void fifo_write_wait(uint32_t value) {
    while(!fifo_has_space(fifo_in_csr));
    write_u32(fifo_in, value);
}

void fifo_write_string(const char *str) {
    size_t len = strlen(str);
    size_t i;
    
    if(len > 63) len = 63;
    fifo_write_wait(len);
    
    for(i = 0; i < len; i++) {
        fifo_write_wait((uint32_t)(unsigned char)str[i]);
    }
}

void check_nios_commands(void) {
    if(fifo2_has_data(fifo2_out_csr)) {
        uint32_t cmd = read_u32(fifo2_out);
        
        if(cmd == CMD_NEXT) {
            printf("\n[CMD] NEXT\n");
            skip_requested = 1;
            skip_direction = 1;
        }
        else if(cmd == CMD_PREV) {
            printf("\n[CMD] PREV\n");
            skip_requested = 1;
            skip_direction = -1;
        }
    }
}

// ============ Parsing de WAV ============

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

int parse_wav_file(const char *filepath, Song *song) {
    FILE *f;
    char riff[4], wave[4];
    uint32_t file_size;
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    uint16_t audio_format;
    
    f = fopen(filepath, "rb");
    if(!f) {
        printf("  Cannot open: %s\n", filepath);
        return 0;
    }
    
    strcpy(song->meta.title, "Unknown");
    strcpy(song->meta.artist, "Unknown");
    strcpy(song->meta.album, "Unknown");
    strncpy(song->filepath, filepath, MAX_PATH - 1);
    
    fread(riff, 1, 4, f);
    fread(&file_size, 4, 1, f);
    fread(wave, 1, 4, f);
    
    if(strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0) {
        printf("  Not a WAV: %s\n", filepath);
        fclose(f);
        return 0;
    }
    
    while(read_chunk_header(f, chunk_id, &chunk_size)) {
        chunk_id[4] = '\0';
        
        if(strncmp(chunk_id, "fmt ", 4) == 0) {
            long fmt_start = ftell(f);
            fread(&audio_format, 2, 1, f);
            fread(&song->num_channels, 2, 1, f);
            fread(&song->sample_rate, 4, 1, f);
            fseek(f, 4, SEEK_CUR);
            fseek(f, 2, SEEK_CUR);
            fread(&song->bits_per_sample, 2, 1, f);
            fseek(f, fmt_start + chunk_size, SEEK_SET);
        }
        else if(strncmp(chunk_id, "LIST", 4) == 0) {
            parse_list_chunk(f, chunk_size, &song->meta);
        }
        else if(strncmp(chunk_id, "data", 4) == 0) {
            song->data_size = chunk_size;
            song->data_offset = ftell(f);
            fclose(f);
            
            if(audio_format != 1 && audio_format != 65534) {
                printf("  Not PCM: %s (format=%d)\n", filepath, audio_format);
                return 0;
            }
            if(song->bits_per_sample != 16) {
                printf("  Not 16-bit: %s (%d-bit)\n", filepath, song->bits_per_sample);
                return 0;
            }
            return 1;
        }
        else {
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);
        }
    }
    
    printf("  No data chunk: %s\n", filepath);
    fclose(f);
    return 0;
}

// ============ Cargar playlist ============

int is_directory(const char *path) {
    struct stat st;
    if(stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

int add_song_to_playlist(const char *filepath) {
    if(playlist_count >= MAX_SONGS) return 0;
    
    if(parse_wav_file(filepath, &playlist[playlist_count])) {
        printf("  [%d] %s - %s (%u Hz)\n", 
               playlist_count + 1,
               playlist[playlist_count].meta.artist,
               playlist[playlist_count].meta.title,
               playlist[playlist_count].sample_rate);
        playlist_count++;
        return 1;
    }
    return 0;
}

int load_playlist_from_dir(const char *directory) {
    DIR *dir;
    struct dirent *entry;
    char filepath[MAX_PATH];
    
    dir = opendir(directory);
    if(!dir) {
        perror("Error opening directory");
        return 0;
    }
    
    while((entry = readdir(dir)) != NULL && playlist_count < MAX_SONGS) {
        size_t len = strlen(entry->d_name);
        
        if(len > 4 && strcasecmp(entry->d_name + len - 4, ".wav") == 0) {
            snprintf(filepath, MAX_PATH, "%s/%s", directory, entry->d_name);
            add_song_to_playlist(filepath);
        }
    }
    
    closedir(dir);
    return playlist_count;
}

// ============ Reproducir canción ============

int play_song(int index) {
    FILE *f;
    Song *song;
    int16_t *audio_buffer;
    size_t buffer_samples = 2048;
    size_t buffer_size, samples_read, i;
    
    if(index < 0 || index >= playlist_count) return 0;
    
    song = &playlist[index];
    
    printf("\n========================================\n");
    printf("Playing [%d/%d]: %s\n", index + 1, playlist_count, song->meta.title);
    printf("Artist: %s | Album: %s\n", song->meta.artist, song->meta.album);
    printf("Rate: %u Hz | Channels: %u\n", song->sample_rate, song->num_channels);
    printf("========================================\n");
    
    f = fopen(song->filepath, "rb");
    if(!f) return 0;
    fseek(f, song->data_offset, SEEK_SET);
    
    printf("Sending METADATA_MAGIC...\n");
    fflush(stdout);
    fifo_write_wait(METADATA_MAGIC);
    
    printf("Sending sample_rate: %u\n", song->sample_rate);
    fflush(stdout);
    fifo_write_wait(song->sample_rate);
    
    printf("Sending num_channels: %u\n", song->num_channels);
    fflush(stdout);
    fifo_write_wait(song->num_channels);
    
    printf("Sending bits_per_sample: %u\n", song->bits_per_sample);
    fflush(stdout);
    fifo_write_wait(song->bits_per_sample);
    
    printf("Sending artist: '%s' (len=%zu)\n", song->meta.artist, strlen(song->meta.artist));
    fflush(stdout);
    fifo_write_string(song->meta.artist);
    printf("  artist sent OK\n");
    fflush(stdout);
    
    printf("Sending album: '%s' (len=%zu)\n", song->meta.album, strlen(song->meta.album));
    fflush(stdout);
    fifo_write_string(song->meta.album);
    printf("  album sent OK\n");
    fflush(stdout);
    
    printf("Sending title: '%s' (len=%zu)\n", song->meta.title, strlen(song->meta.title));
    fflush(stdout);
    fifo_write_string(song->meta.title);
    printf("  title sent OK\n");
    fflush(stdout);
    
    printf("All metadata sent, starting audio...\n");
    fflush(stdout);
    
    printf("Metadata sent, starting audio...\n");
    
    buffer_size = buffer_samples * song->num_channels;
    audio_buffer = (int16_t *)malloc(buffer_size * sizeof(int16_t));
    if(!audio_buffer) {
        fclose(f);
        return 0;
    }
    
    skip_requested = 0;
    
    while((samples_read = fread(audio_buffer, sizeof(int16_t), buffer_size, f)) > 0) {
        check_nios_commands();
        if(skip_requested) break;
        
        if(song->num_channels == 2) {
            for(i = 0; i < samples_read && !skip_requested; i += 2) {
                int32_t mono = ((int32_t)audio_buffer[i] + audio_buffer[i+1]) / 2;
                
                while(!fifo_has_space(fifo_in_csr)) {
                    check_nios_commands();
                    if(skip_requested) break;
                }
                if(skip_requested) break;
                
                write_u32(fifo_in, (uint32_t)(mono & 0xFFFF));
            }
        } else {
            for(i = 0; i < samples_read && !skip_requested; i++) {
                while(!fifo_has_space(fifo_in_csr)) {
                    check_nios_commands();
                    if(skip_requested) break;
                }
                if(skip_requested) break;
                
                write_u32(fifo_in, (uint32_t)(audio_buffer[i] & 0xFFFF));
            }
        }
    }
    
    free(audio_buffer);
    fclose(f);
    return 1;
}

// ============ Main ============

int main(int argc, char *argv[]) {
    int fd_mem, i;
    
    printf("========================================\n");
    printf("  WAV Playlist Player\n");
    printf("========================================\n\n");
    
    if(argc < 2) {
        printf("Usage: %s <file1.wav> [file2.wav ...]\n", argv[0]);
        printf("   or: %s <directory>\n", argv[0]);
        return 1;
    }
    
    // Cargar canciones
    printf("Loading songs...\n");
    
    if(is_directory(argv[1])) {
        load_playlist_from_dir(argv[1]);
    } else {
        for(i = 1; i < argc; i++) {
            add_song_to_playlist(argv[i]);
        }
    }
    
    if(playlist_count == 0) {
        fprintf(stderr, "No valid WAV files found!\n");
        return 1;
    }
    
    printf("\nLoaded %d song(s).\n", playlist_count);
    
    // Mapear memoria
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        perror("Error opening /dev/mem");
        return 1;
    }
    
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, 
                   MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    
    if(h2f_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd_mem);
        return 1;
    }
    
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;
    fifo2_out = h2f_map + FIFO2_OUT_OFFSET;
    fifo2_out_csr = h2f_map + FIFO2_OUT_CSR_OFFSET;
    
    printf("\nKEY3=Pause | KEY2=Next | KEY1=Prev\n\n");
    
    // Loop de playlist
    while(1) {
        play_song(current_song);
        
        if(skip_requested) {
            current_song += skip_direction;
            skip_requested = 0;
        } else {
            current_song++;
        }
        
        if(current_song >= playlist_count) current_song = 0;
        if(current_song < 0) current_song = playlist_count - 1;
        
        usleep(300000);
    }
    
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    
    return 0;
}