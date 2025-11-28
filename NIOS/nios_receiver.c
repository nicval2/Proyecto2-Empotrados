// nios_receiver.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>

// Según tus imágenes
#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880

#define FIFO_EMPTY (1 << 0)

int main() {
    alt_u32 data;
    alt_u32 count = 0;
    alt_u32 status;
    
    printf("\n========================================\n");
    printf("  NIOS II - Waiting for ARM data\n");
    printf("========================================\n\n");
    
    status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
    printf("Initial FIFO status: 0x%08X\n", status);
    printf("  EMPTY: %s\n\n", (status & FIFO_EMPTY) ? "YES" : "NO");
    
    printf("Listening...\n");
    printf("-----------------------------------\n");
    
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            count++;
            
            printf("[%02d] RX: 0x%08X", count, data);
            
            if((data & 0xFFFF0000) == 0xAA550000) {
                printf(" <- VALID!\n");
            } else {
                printf(" <- unexpected\n");
            }
        }
        
        usleep(10000);
    }
    
    return 0;
}