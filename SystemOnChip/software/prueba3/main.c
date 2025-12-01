// main.c - NIOS II (v2.2 - Sin flush inmediato)
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "7_segments.h"
#include "buttons.h"
#include "switches.h"
#include "timer.h"
#include "filter.h"
#include "audio.h"
#include "vga.h"

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

void update_vga_timer(alt_u32 seconds) {
    char buf[16];
    sprintf(buf, "%02lu:%02lu", seconds / 60, seconds % 60);
    vga_print_center(buf, 15);
}

void show_now_playing(void) {
    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 2);
    vga_print_center(title_str, 5);
    vga_print_center(artist_str, 7);
    vga_print_center(album_str, 9);
    vga_print_center("TIME", 13);
    update_vga_timer(0);
    vga_print_center("[KEY3:Pause] [KEY2:Next] [KEY1:Prev]", 25);
}

int main()
{
    alt_putstr("\n=== REPRODUCTOR NIOS v2.2 ===\n");

    // Inicializacion
    buttons_init();
    timer_init();
    filter_init();
    audio_init();
    vga_init();

    display_time_4seg(SEG7_PTR, 0, 0);

    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 5);
    vga_print_center("Esperando HPS...", 7);

    // === BUCLE PRINCIPAL ===
    while(1)
    {
        // Resetear contadores
        alt_u32 sample_count = 0;
        alt_u32 seconds_total = 0;
        display_time_4seg(SEG7_PTR, 0, 0);

        // Limpiar flags de skip de cancion anterior
        buttons_clear_skip();

        // Esperar nueva cancion del HPS
        vga_clear();
        vga_print_center("Esperando siguiente pista...", 10);
        audio_wait_handshake();

        // Recibir metadata
        alt_u32 sample_rate = audio_receive_metadata();

        printf("Reproduciendo: %s\n", title_str);
        show_now_playing();

        int song_active = 1;

        // === BUCLE DE CANCION ===
        while (song_active)
        {
            // A. Switches (Filtros/Volumen)
            int sw = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

            if(sw & 0x01) filter_set(FILTER_LOWPASS);
            else if(sw & 0x02) filter_set(FILTER_HIGHPASS);
            else filter_set(FILTER_NONE);

            volume_shift = (sw & 0x200) ? 2 : 0;

            // B. Procesar Audio
            if (is_running) {
                if (audio_fifo_has_data()) {
                    int status = audio_process_sample();

                    if (status == 0) {
                        // EOS normal
                        printf("Fin de cancion\n");
                        song_active = 0;
                    }
                    else if (status == -1) {
                        // SKIP recibido del HPS
                        printf("Skip recibido\n");
                        song_active = 0;
                    }
                    else {
                        // Audio procesado OK
                        sample_count++;
                        if(sample_count >= sample_rate) {
                            seconds_total++;
                            sample_count = 0;
                            display_time_4seg(SEG7_PTR, seconds_total / 60, seconds_total % 60);
                            update_vga_timer(seconds_total);
                        }
                    }
                }
            }
            else {
                // Pausado
                usleep(5000);
            }
        }

        // Mostrar transicion
        vga_clear_line(20);
        vga_print_center(">> Cambiando pista... <<", 20);
        usleep(100000);

    } // Fin while(1)

    return 0;
}
