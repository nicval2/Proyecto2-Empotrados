// test_fifo2_arm.c
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

#define H2F_BRIDGE_BASE     0xC0000000

// FIFO2 (NIOS -> ARM) - Recibir
#define FIFO2_OUT_OFFSET     0x8950
#define FIFO2_OUT_CSR_OFFSET 0x8960

#define FIFO_EMPTY (1 << 0)

uint32_t read_u32(volatile void *addr) {
    return *(volatile uint32_t *)addr;
}

int fifo2_has_data(volatile void *fifo_csr) {
    uint32_t status = read_u32(fifo_csr);
    return !(status & FIFO_EMPTY);
}

int main() {
    int fd_mem;
    void *h2f_map;
    volatile void *fifo2_out;
    volatile void *fifo2_out_csr;
    uint32_t data;
    int count = 0;
    
    printf("========================================\n");
    printf("  FIFO2 Test: ARM receives from NIOS\n");
    printf("========================================\n\n");
    
    // Abrir /dev/mem
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if(fd_mem == -1) {
        perror("Error opening /dev/mem");
        return 1;
    }
    
    // Mapear memoria
    h2f_map = mmap(NULL, 0x10000, PROT_READ | PROT_WRITE, 
                   MAP_SHARED, fd_mem, H2F_BRIDGE_BASE);
    
    if(h2f_map == MAP_FAILED) {
        perror("mmap failed");
        close(fd_mem);
        return 1;
    }
    
    fifo2_out = h2f_map + FIFO2_OUT_OFFSET;
    fifo2_out_csr = h2f_map + FIFO2_OUT_CSR_OFFSET;
    
    printf("FIFO2_OUT: 0x%08X\n", H2F_BRIDGE_BASE + FIFO2_OUT_OFFSET);
    printf("FIFO2_CSR: 0x%08X\n\n", H2F_BRIDGE_BASE + FIFO2_OUT_CSR_OFFSET);
    
    printf("Waiting for data from NIOS...\n");
    printf("(Run the NIOS program first)\n\n");
    
    // Esperar y leer datos
    while(count < 15) {
        if(fifo2_has_data(fifo2_out_csr)) {
            data = read_u32(fifo2_out);
            
            if(count == 0 && data == 0xCAFEBABE) {
                printf("Received MAGIC: 0x%08X - Connection OK!\n", data);
            } else {
                printf("Received[%d]: %u (0x%08X)\n", count, data, data);
            }
            
            count++;
        }
        
        usleep(100000);  // 100ms
    }
    
    printf("\n========================================\n");
    printf("Test complete! Received %d values.\n", count);
    printf("========================================\n");
    
    munmap(h2f_map, 0x10000);
    close(fd_mem);
    
    return 0;
}