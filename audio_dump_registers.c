// audio_dump_registers.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>

#define AUDIO_BASE 0x8920

int main() {
    printf("\n========================================\n");
    printf("  Audio Codec Register Dump\n");
    printf("========================================\n\n");
    
    printf("Audio Base: 0x%08X\n\n", AUDIO_BASE);
    
    printf("Register dump (offsets 0-20):\n");
    printf("Offset | Address    | Value\n");
    printf("-------|------------|----------\n");
    
    for(int offset = 0; offset < 20; offset += 4) {
        alt_u32 addr = AUDIO_BASE + offset;
        alt_u32 value = IORD_32DIRECT(addr, 0);
        
        printf("  +%2d  | 0x%08X | 0x%08X", 
               offset, addr, (unsigned int)value);
        
        // Anotar offsets importantes
        if(offset == 0) printf(" <- Possible data");
        if(offset == 4) printf(" <- Possible FIFO space");
        if(offset == 8) printf(" <- Possible left data");
        if(offset == 12) printf(" <- Possible right data");
        
        printf("\n");
    }
    
    printf("\n========================================\n\n");
    
    return 0;
}