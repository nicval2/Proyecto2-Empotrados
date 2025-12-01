// main.c - NIOS II con Ecualizador (v2.3)
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "7_segments.h"
#include "buttons.h"
#include "filter.h"
#include "audio.h"
#include "vga.h"

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

/* Variables globales para display */
static char current_filter_name[16] = "NORMAL";

void update_vga_timer(alt_u32 seconds) {
    char buf[16];
    sprintf(buf, "%02lu:%02lu", seconds / 60, seconds % 60);
    vga_print_center(buf, 15);
}

void update_vga_filter(void) {
    /* Limpiar línea anterior */
    vga_clear_line(17);

    /* Mostrar filtro actual */
    char buf[32];
    sprintf(buf, "EQ: %s", filter_get_name());
    vga_print_center(buf, 17);
}

void show_waiting_screen(void) {
    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 3);
    vga_print_center("Esperando HPS...", 7);
    vga_print_center("", 10);
    vga_print_center("CONTROLES:", 12);
    vga_print_center("KEY3: Play/Pause", 14);
    vga_print_center("KEY2: Siguiente", 15);
    vga_print_center("KEY1: Anterior", 16);
    vga_print_center("", 18);
    vga_print_center("ECUALIZADOR (Switches):", 20);
    vga_print_center("SW0:Bass+ SW1:Bass- SW2:Treble+", 22);
    vga_print_center("SW3:Treble- SW4:Vocal SW5:Rock", 23);
    vga_print_center("SW6:Pop SW7:Jazz SW9:Vol-", 24);
}

void show_now_playing(void) {
    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 2);
    vga_print_center(title_str, 5);
    vga_print_center(artist_str, 7);
    vga_print_center(album_str, 9);
    vga_print_center("", 12);
    vga_print_center("TIME:", 13);
    update_vga_timer(0);
    vga_print_center("", 16);
    update_vga_filter();
    vga_print_center("", 19);
    vga_print_center("[KEY3:Pause] [KEY2:Next] [KEY1:Prev]", 25);
    vga_print_center("SW0-7: Ecualizador  SW9: Volumen", 27);
}

int main()
{
    alt_putstr("\n=== REPRODUCTOR NIOS v2.3 ===\n");
    alt_putstr("Con Ecualizador de Audio\n\n");

    /* Inicialización de Hardware */
    buttons_init();
    filter_init();
    audio_init();
    vga_init();

    display_time_4seg(SEG7_PTR, 0, 0);
    show_waiting_screen();

    /* === BUCLE PRINCIPAL === */
    while(1)
    {
        /* Resetear contadores */
        alt_u32 sample_count = 0;
        alt_u32 seconds_total = 0;
        display_time_4seg(SEG7_PTR, 0, 0);

        /* Limpiar flags de skip */
        buttons_clear_skip();

        /* Esperar nueva canción del HPS */
        vga_clear();
        vga_print_center("Esperando siguiente pista...", 10);
        audio_wait_handshake();

        /* Recibir metadata */
        alt_u32 sample_rate = audio_receive_metadata();

        printf("Reproduciendo: %s - %s\n", artist_str, title_str);
        show_now_playing();

        int song_active = 1;
        int last_sw = -1;

        /* === BUCLE DE CANCIÓN === */
        while (song_active)
        {
            /* A. Leer Switches */
            int sw = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

            /* B. Actualizar ecualizador si cambió */
            if (sw != last_sw) {
                /* Bits 0-7: Ecualizador */
                if (filter_update_from_switches(sw & 0xFF)) {
                    printf("Filtro: %s\n", filter_get_name());
                    update_vga_filter();
                }

                /* Bit 9: Volumen */
                volume_shift = (sw & 0x200) ? 2 : 0;

                last_sw = sw;
            }

            /* C. Procesar Audio */
            if (is_running) {
                if (audio_fifo_has_data()) {
                    int status = audio_process_sample();

                    if (status == 0) {
                        /* EOS normal */
                        printf("Fin de cancion\n");
                        song_active = 0;
                    }
                    else if (status == -1) {
                        /* SKIP recibido del HPS */
                        printf("Skip recibido\n");
                        song_active = 0;
                    }
                    else {
                        /* Audio procesado OK - actualizar tiempo */
                        sample_count++;
                        if (sample_count >= sample_rate) {
                            seconds_total++;
                            sample_count = 0;
                            display_time_4seg(SEG7_PTR,
                                seconds_total / 60,
                                seconds_total % 60);
                            update_vga_timer(seconds_total);
                        }
                    }
                }
            }
            else {
                /* Pausado */
                usleep(5000);
            }
        }

        /* Mostrar transición */
        vga_clear_line(20);
        vga_print_center(">> Cambiando pista... <<", 20);
        usleep(100000);

    } /* Fin while(1) */

    return 0;
}
