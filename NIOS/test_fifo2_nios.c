// test_fifo2_nios.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

// FIFO (ARM -> NIOS) - Recibir
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880

// FIFO2 (NIOS -> ARM) - Enviar
#define FIFO2_IN_BASE     0x8940
#define FIFO2_IN_CSR_BASE 0x8980

#define FIFO_EMPTY (1 << 0)
#define FIFO_FULL  (1 << 1)

// Verificar si FIFO2 tiene espacio para escribir
int fifo2_has_space() {
    alt_u32 status = IORD_32DIRECT(FIFO2_IN_CSR_BASE, 0);
    return !(status & FIFO_FULL);
}

// Enviar dato al ARM
void fifo2_write(alt_u32 data) {
    while(!fifo2_has_space());
    IOWR_32DIRECT(FIFO2_IN_BASE, 0, data);
}

int main() {
    alt_u32 counter = 0;
    alt_u32 magic = 0xCAFEBABE;
    
    printf("\n========================================\n");
    printf("  FIFO2 Test: NIOS -> ARM\n");
    printf("========================================\n\n");
    
    printf("Sending test data to ARM...\n");
    printf("FIFO2_IN_BASE: 0x%04X\n", FIFO2_IN_BASE);
    printf("FIFO2_IN_CSR_BASE: 0x%04X\n\n", FIFO2_IN_CSR_BASE);
    
    // Enviar magic number primero
    fifo2_write(magic);
    printf("Sent MAGIC: 0x%08X\n", (unsigned int)magic);
    
    // Enviar contador cada segundo
    while(1) {
        counter++;
        fifo2_write(counter);
        printf("Sent: %u\n", (unsigned int)counter);
        
        usleep(1000000);  // 1 segundo
        
        if(counter >= 10) {
            printf("\nTest complete. Sent 10 values.\n");
            break;
        }
    }
    
    printf("NIOS waiting... (check ARM side)\n");
    
    while(1) {
        usleep(1000000);
    }
    
    return 0;
}