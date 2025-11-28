// test_arm_h2f_final.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// Configuración del puente H2F
#define H2F_BRIDGE_BASE     0xC0000000

// Offsets desde Platform Designer (según tu screenshot)
#define FIFO_IN_OFFSET      0x8860  // FIFO.in
#define FIFO_IN_CSR_OFFSET  0x8900  // FIFO.in_csr

#define PAGE_SIZE 4096
#define FIFO_FULL  (1 << 1)
#define FIFO_EMPTY (1 << 0)

// Funciones para acceso seguro a memoria
uint32_t read_u32(volatile void *addr) {
    uint32_t value;
    memcpy(&value, (void*)addr, sizeof(uint32_t));
    return value;
}

void write_u32(volatile void *addr, uint32_t value) {
    memcpy((void*)addr, &value, sizeof(uint32_t));
}

int main() {
    int fd;
    void *h2f_map;
    volatile void *fifo_in;
    volatile void *fifo_in_csr;
    uint32_t status;
    int i;
    int num_samples = 10;
    
    printf("========================================\n");
    printf("ARM HPS -> NIOS FIFO Test\n");
    printf("========================================\n\n");
    
    // Abrir /dev/mem
    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd == -1) {
        perror("Error opening /dev/mem");
        printf("Run with: sudo\n");
        return -1;
    }
    
    printf("[OK] /dev/mem opened\n");
    
    // Mapear H2F bridge - mapeamos más memoria para cubrir ambos offsets
    h2f_map = mmap(NULL, 
                   0x10000,  // 64KB suficiente
                   PROT_READ | PROT_WRITE, 
                   MAP_SHARED, 
                   fd, 
                   H2F_BRIDGE_BASE);
    
    if(h2f_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }
    
    printf("[OK] H2F Bridge mapped at virtual: %p\n", h2f_map);
    printf("\nPhysical addresses:\n");
    printf("  FIFO_IN:     0x%08X\n", H2F_BRIDGE_BASE + FIFO_IN_OFFSET);
    printf("  FIFO_IN_CSR: 0x%08X\n\n", H2F_BRIDGE_BASE + FIFO_IN_CSR_OFFSET);
    
    // Obtener punteros
    fifo_in = h2f_map + FIFO_IN_OFFSET;
    fifo_in_csr = h2f_map + FIFO_IN_CSR_OFFSET;
    
    // Leer estado inicial
    status = read_u32(fifo_in_csr);
    printf("FIFO Initial Status: 0x%08X\n", status);
    printf("  EMPTY: %s\n", (status & FIFO_EMPTY) ? "YES" : "NO");
    printf("  FULL:  %s\n", (status & FIFO_FULL) ? "YES" : "NO");
    printf("  Fill level: %d words\n\n", (status >> 16) & 0xFFFF);
    
    // Enviar datos de prueba
    printf("Sending %d samples to NIOS...\n", num_samples);
    printf("-----------------------------------\n");
    
    for(i = 0; i < num_samples; i++) {
        uint32_t test_val = 0xDEAD0000 + i;
        
        // Verificar si hay espacio
        status = read_u32(fifo_in_csr);
        if(status & FIFO_FULL) {
            printf("[%02d] FIFO FULL! Waiting...\n", i);
            usleep(100000);
            i--;
            continue;
        }
        
        // Escribir al FIFO
        write_u32(fifo_in, test_val);
        printf("[%02d] Sent: 0x%08X\n", i, test_val);
        
        usleep(300000);  // 300ms entre muestras
    }
    
    printf("-----------------------------------\n");
    printf("[OK] All samples sent!\n\n");
    
    // Estado final
    status = read_u32(fifo_in_csr);
    printf("FIFO Final Status: 0x%08X\n", status);
    printf("  EMPTY: %s\n", (status & FIFO_EMPTY) ? "YES" : "NO");
    printf("  FULL:  %s\n", (status & FIFO_FULL) ? "YES" : "NO");
    printf("  Fill level: %d words\n", (status >> 16) & 0xFFFF);
    
    // Cleanup
    munmap(h2f_map, 0x10000);
    close(fd);
    
    printf("\nTest complete!\n");
    return 0;
}