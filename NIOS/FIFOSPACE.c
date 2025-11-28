#include <stdio.h>
#include <io.h>

#define AUDIO_BASE 0x8920

int main() {
    printf("Audio Core Test\n");
    printf("================\n\n");
    
    while(1) {
        alt_u32 fifospace = IORD_32DIRECT(AUDIO_BASE, 4);
        
        alt_u8 wslc = (fifospace >> 24) & 0xFF;
        alt_u8 wsrc = (fifospace >> 16) & 0xFF;
        
        printf("FIFOSPACE: 0x%08X  |  Write L: %3u  R: %3u\n", 
               (unsigned int)fifospace, wslc, wsrc);
        
        usleep(500000);  // 0.5 segundos
    }
    
    return 0;
}
