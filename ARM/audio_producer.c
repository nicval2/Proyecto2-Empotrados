/*
 * audio_producer.c
 * ACTUALIZADO PARA NUEVAS DIRECCIONES DE MEMORIA
 * FIFO Data: 0x8860 | FIFO CSR: 0x8900
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

// ==========================================
// 1. CONFIGURACIÓN DE DIRECCIONES (ACTUALIZADO)
// ==========================================
#define HPS_FPGA_BASE_ADDR     0xC0000000

// --- TUS NUEVAS DIRECCIONES DE PLATFORM DESIGNER ---
#define FIFO_DATA_OFFSET_RAW   0x8860      // Nuevo offset de FIFO.in
#define FIFO_CSR_OFFSET_RAW    0x8900      // Nuevo offset de FIFO.in_csr

// --- CÁLCULO DE ALINEACIÓN DE PÁGINA (Automático) ---
// La página debe estar alineada a 4096 bytes (0x1000)
#define PAGE_SIZE              0x1000
#define PAGE_MASK              (~(PAGE_SIZE - 1))

// Dirección física alineada donde comienza la página (0xC0008000)
#define ALIGNED_PHY_ADDR       (HPS_FPGA_BASE_ADDR + (FIFO_DATA_OFFSET_RAW & PAGE_MASK))

// Offsets relativos dentro de la página mapeada
// Esto calcula automáticamente la distancia desde 0x8000 hasta 0x8860 y 0x8900
#define DATA_OFFSET_IN_PAGE    ((HPS_FPGA_BASE_ADDR + FIFO_DATA_OFFSET_RAW) - ALIGNED_PHY_ADDR)
#define CSR_OFFSET_IN_PAGE     ((HPS_FPGA_BASE_ADDR + FIFO_CSR_OFFSET_RAW) - ALIGNED_PHY_ADDR)

// Configuración del FIFO
#define FIFO_DEPTH             2048        // Ajustar según configuración del IP
#define FIFO_CSR_LEVEL_MASK    0x0000FFFF  // Máscara típica para leer nivel

// Estructura WAV
typedef struct {
    char chunkID[4];
    uint32_t chunkSize;
    char format[4];
    char subchunk1ID[4];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} WavHeader;

// Punteros globales
volatile uint8_t *virtual_base = NULL;
volatile uint32_t *fifo_data_ptr = NULL;
volatile uint32_t *fifo_csr_ptr = NULL;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <archivo.wav>\n", argv[0]);
        return 1;
    }

    // ---------------------------------------------------------
    // 1. MAPEO DE MEMORIA
    // ---------------------------------------------------------
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd == -1) {
        perror("Error abriendo /dev/mem");
        return 1;
    }

    // Mapeamos la página que comienza en 0xC0008000
    virtual_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, ALIGNED_PHY_ADDR);
    if (virtual_base == MAP_FAILED) {
        perror("Error en mmap");
        close(fd);
        return 1;
    }

    // Asignamos los punteros sumando los offsets calculados
    fifo_data_ptr = (volatile uint32_t *)(virtual_base + DATA_OFFSET_IN_PAGE);
    fifo_csr_ptr  = (volatile uint32_t *)(virtual_base + CSR_OFFSET_IN_PAGE);

    printf("HPS: Memoria mapeada correctamente.\n");
    printf("     Pagina Base Virtual: %p\n", virtual_base);
    printf("     FIFO Data (Virtual): %p (Offset fisico 0x%X)\n", fifo_data_ptr, FIFO_DATA_OFFSET_RAW);
    printf("     FIFO CSR  (Virtual): %p (Offset fisico 0x%X)\n", fifo_csr_ptr, FIFO_CSR_OFFSET_RAW);

    // ---------------------------------------------------------
    // 2. LECTURA Y ENVÍO DEL WAV
    // ---------------------------------------------------------
    FILE *wav_file = fopen(argv[1], "rb");
    if (!wav_file) {
        perror("Error abriendo archivo WAV");
        return 1;
    }

    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, wav_file) < 1) {
         printf("Error leyendo header.\n"); return 1;
    }
    
    // Saltar cabecera (44 bytes) para ir a los datos crudos
    fseek(wav_file, 44, SEEK_SET); 

    printf("Reproduciendo: %s (%u Hz)\n", argv[1], header.sampleRate);
    printf("Enviando datos al FIFO... (Presione Ctrl+C para detener)\n");

    uint32_t buffer[256]; 
    size_t words_read;
    
    while ((words_read = fread(buffer, 4, 256, wav_file)) > 0) {
        
        // --- CONTROL DE FLUJO ---
        // Leer nivel de llenado del FIFO
        uint32_t fill_level = (*fifo_csr_ptr & FIFO_CSR_LEVEL_MASK);
        
        // Si el FIFO está casi lleno, esperar para no desbordar
        while (fill_level > (FIFO_DEPTH - 200)) {
            // Espera activa o usleep muy breve
            usleep(500); 
            fill_level = (*fifo_csr_ptr & FIFO_CSR_LEVEL_MASK);
        }

        // --- ESCRITURA ---
        for (int i = 0; i < words_read; i++) {
            *fifo_data_ptr = buffer[i];
        }
    }

    printf("\nFin de la cancion.\n");

    fclose(wav_file);
    munmap((void*)virtual_base, PAGE_SIZE);
    close(fd);
    return 0;
}