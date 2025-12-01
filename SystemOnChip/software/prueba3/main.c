// main.c - NIOS II
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

void update_vga_timer_display(alt_u32 total_seconds) {
    char time_buf[16];
    sprintf(time_buf, "%02lu:%02lu", total_seconds / 60, total_seconds % 60);
    vga_print_center(time_buf, 15);
}

void show_waiting_screen(void) {
    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 5);
    vga_print_center("Esperando conexion HPS...", 7);
    vga_print_center("KEY3: Play/Pause", 20);
    vga_print_center("KEY2: Siguiente", 21);
    vga_print_center("KEY1: Anterior", 22);
}

void show_now_playing(void) {
    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 2);
    vga_print_center(title_str, 5);
    vga_print_center(artist_str, 7);
    vga_print_center(album_str, 9);
    vga_print_center("TIME", 13);
    update_vga_timer_display(0);

    // Controles en la parte inferior
    vga_print_center("[KEY3:Play/Pause] [KEY2:Next] [KEY1:Prev]", 25);
}

int main()
{
    alt_putstr("\n--- SISTEMA SOC REPRODUCTOR v2.0 ---\n");
    alt_putstr("Con soporte Next/Prev\n\n");

    // Inicialización de Hardware
    buttons_init();
    timer_init();
    filter_init();
    audio_init();
    vga_init();

    display_time_4seg(SEG7_PTR, 0, 0);
    show_waiting_screen();

    // === BUCLE PRINCIPAL (Playlist Loop) ===
    while(1)
    {
        // 1. Resetear contadores para nueva canción
        alt_u32 sample_count = 0;
        alt_u32 seconds_total = 0;
        display_time_4seg(SEG7_PTR, 0, 0);

        // Limpiar cualquier skip pendiente de la canción anterior
        buttons_clear_skip();

        // 2. Esperar Handshake del HPS (bloquea hasta recibir METADATA_MAGIC)
        show_waiting_screen();
        audio_wait_handshake();

        // 3. Recibir Metadata
        alt_u32 sample_rate = audio_receive_metadata();

        printf("\n>>> REPRODUCIENDO <<<\n");
        printf("Titulo:  %s\n", title_str);
        printf("Artista: %s\n", artist_str);
        printf("Album:   %s\n", album_str);

        // 4. Actualizar pantalla
        show_now_playing();

        // Flag para el bucle de la canción actual
        int song_finished = 0;

        // === BUCLE DE CANCIÓN ACTUAL ===
        while (!song_finished)
        {
            /* A. Procesar Switches (Filtros/Volumen) */
            int sw_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

            // SW0: Low-pass, SW1: High-pass
            if(sw_val & 0x01)
                filter_set(FILTER_LOWPASS);
            else if(sw_val & 0x02)
                filter_set(FILTER_HIGHPASS);
            else
                filter_set(FILTER_NONE);

            // SW9: Volumen bajo
            if (sw_val & 0x200)
                volume_shift = 2;
            else
                volume_shift = 0;

            /* B. Verificar si hubo solicitud de skip (Next/Prev) */
            if (skip_request) {
                printf("Skip solicitado (dir=%d)\n", skip_direction);

                // El comando ya fue enviado al HPS desde la ISR
                // Ahora debemos vaciar el FIFO hasta recibir SKIP_TOKEN
                audio_flush_fifo();

                // Marcar canción como terminada para salir del bucle
                song_finished = 1;
                buttons_clear_skip();
                continue;  // Saltar al siguiente ciclo del while externo
            }

            /* C. Reset de tiempo (opcional, KEY1 sin skip) */
            if (reset_request && !skip_request) {
                sample_count = 0;
                seconds_total = 0;
                display_time_4seg(SEG7_PTR, 0, 0);
                update_vga_timer_display(0);
                reset_request = 0;
            }

            /* D. Procesamiento de Audio */
            if (is_running) {
                if (audio_fifo_has_data()) {
                    int status = audio_process_sample();

                    if (status == 0) {
                        // EOS normal - fin de canción
                        printf("Fin de cancion (EOS)\n");
                        song_finished = 1;
                    }
                    else if (status == -1) {
                        // SKIP token - el HPS está cambiando de canción
                        printf("Skip recibido del HPS\n");
                        song_finished = 1;
                    }
                    else {
                        // Audio normal procesado
                        sample_count++;
                        if(sample_count >= sample_rate) {
                            seconds_total++;
                            sample_count = 0;
                            display_time_4seg(SEG7_PTR, seconds_total / 60, seconds_total % 60);
                            update_vga_timer_display(seconds_total);
                        }
                    }
                }
            }
            else {
                // Pausado - pequeña espera para no saturar CPU
                usleep(1000);
            }
        }

        printf("Cancion finalizada. Esperando siguiente...\n");
        vga_clear_line(20);
        vga_print_center(">> SIGUIENTE PISTA... <<", 20);

        // Pequeña pausa visual
        usleep(200000);

    } // Fin del while(1) Playlist

    return 0;
}
