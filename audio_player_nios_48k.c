// audio_player_nios_48k.c
#include <stdio.h>
#include <system.h>
#include <io.h>
#include <alt_types.h>
#include <unistd.h>

#define FIFO_OUT_BASE     0x8870
#define FIFO_OUT_CSR_BASE 0x8880
#define AUDIO_BASE        0x8920
#define AUDIO_FIFO_SPACE  (AUDIO_BASE + 4)
#define AUDIO_LEFT_DATA   (AUDIO_BASE + 8)
#define AUDIO_RIGHT_DATA  (AUDIO_BASE + 12)

#define FIFO_EMPTY (1 << 0)
#define METADATA_MAGIC 0xDEADDA7A

alt_u32 sample_rate = 0;
alt_u32 num_channels = 0;
alt_u32 bits_per_sample = 0;

void wait_for_audio_fifo() {
    volatile alt_u32 *fifo_space = (alt_u32 *)AUDIO_FIFO_SPACE;
    while((*fifo_space & 0xFF000000) == 0);
}

void send_audio_sample(alt_u32 sample) {
    volatile alt_u32 *audio_left = (alt_u32 *)AUDIO_LEFT_DATA;
    volatile alt_u32 *audio_right = (alt_u32 *)AUDIO_RIGHT_DATA;
    
    wait_for_audio_fifo();
    
    // Sample es 16-bit signed (viene como 0x0000XXXX)
    alt_16 sample_16 = (alt_16)(sample & 0xFFFF);
    
    // Left Justified 32-bit: sample en bits [31:16]
    alt_32 sample_32 = ((alt_32)sample_16) << 16;
    
    *audio_left = sample_32;
    *audio_right = sample_32;
}

int main() {
    alt_u32 data, status;
    alt_u32 count = 0;
    alt_u32 seconds = 0;
    int metadata_ok = 0;
    
    printf("\n========================================\n");
    printf("  NIOS II WAV Player (48kHz Fixed)\n");
    printf("========================================\n\n");
    
    printf("Audio Configuration:\n");
    printf("  Codec Sample Rate: 48000 Hz\n");
    printf("  Data Format: Left Justified 32-bit\n");
    printf("  Addresses:\n");
    printf("    FIFO Space: 0x%08X\n", AUDIO_FIFO_SPACE);
    printf("    Left Data:  0x%08X\n", AUDIO_LEFT_DATA);
    printf("    Right Data: 0x%08X\n\n", AUDIO_RIGHT_DATA);
    
    printf("Waiting for metadata from ARM...\n");
    
    while(!metadata_ok) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            
            if(data == METADATA_MAGIC) {
                printf("Metadata received!\n");
                
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                sample_rate = IORD_32DIRECT(FIFO_OUT_BASE, 0);
                
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                num_channels = IORD_32DIRECT(FIFO_OUT_BASE, 0);
                
                while(IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0) & FIFO_EMPTY);
                bits_per_sample = IORD_32DIRECT(FIFO_OUT_BASE, 0);
                
                printf("  File Sample Rate: %u Hz\n", (unsigned int)sample_rate);
                printf("  Channels: %u\n", (unsigned int)num_channels);
                printf("  Bits/Sample: %u\n\n", (unsigned int)bits_per_sample);
                
                if(sample_rate != 48000) {
                    printf("WARNING: File is not 48kHz!\n");
                    printf("Audio will play at wrong speed.\n\n");
                }
                
                metadata_ok = 1;
            }
        }
        usleep(1000);
    }
    
    printf("Starting playback at 48kHz...\n");
    printf("Listen on Line-Out (Green Port)\n");
    printf("-----------------------------------\n");
    
    while(1) {
        status = IORD_32DIRECT(FIFO_OUT_CSR_BASE, 0);
        
        if(!(status & FIFO_EMPTY)) {
            data = IORD_32DIRECT(FIFO_OUT_BASE, 0);
            send_audio_sample(data);
            
            count++;
            
            if(count >= sample_rate) {
                seconds++;
                printf("Playing: %u seconds\n", (unsigned int)seconds);
                count = 0;
            }
        }
    }
    
    return 0;
}