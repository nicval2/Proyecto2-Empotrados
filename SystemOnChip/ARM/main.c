// main.c
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
    sprintf(time_buf, "%02ld:%02ld", total_seconds / 60, total_seconds % 60);
    vga_print_center(time_buf, 15);
}

int main()
{
    alt_putstr("\n--- SISTEMA SOC REPRODUCTOR ---\n");

    // Inicializaci�n de Hardware
    buttons_init();
    timer_init();
    filter_init();
    audio_init();
    vga_init();

    display_time_4seg(SEG7_PTR, 0, 0);

    // Pantalla de Inicio
    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 5);
    vga_print_center("Esperando conexion HPS...", 7);

    // --- BUCLE DE LISTA DE REPRODUCCI�N (Playlist Loop) ---
    while(1)
    {
        // 1. Resetear contadores visuales para nueva canci�n
        alt_u32 sample_count = 0;
        alt_u32 seconds_total = 0;
        display_time_4seg(SEG7_PTR, 0, 0);

        // 2. Esperar Handshake del HPS (Magic Token)
        // Esto bloquear� hasta que el HPS empiece a enviar la siguiente canci�n
        audio_wait_handshake();

        // 3. Recibir Metadata
        alt_u32 sample_rate = audio_receive_metadata();

        printf("\n>>> REPRODUCIENDO NUEVA PISTA <<<\n");
        printf("Titulo: %s\n", title_str);

        // Actualizar GUI
        vga_clear();
        vga_print_center("=== NOW PLAYING ===", 2);
        vga_print_center(title_str, 5);
        vga_print_center(artist_str, 7);
        vga_print_center(album_str, 9);
        vga_print_center("TIME", 13);
        update_vga_timer_display(0);

        // Flag para el bucle de la canci�n actual
        int song_finished = 0;

        // --- BUCLE DE CANCI�N ACTUAL (Song Loop) ---
        while (!song_finished)
        {
            /* A. Switches (Filtros/Volumen) */
            int sw_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);
            if(sw_val & 0x01) filter_set(FILTER_LOWPASS);
            else if(sw_val & 0x02) filter_set(FILTER_HIGHPASS);
            else filter_set(FILTER_NONE);

            if (sw_val & 0x200) volume_shift = 2;
            else volume_shift = 0;

            /* B. Botones (Reset/Next) */
            // NOTA: Para implementar "Next" real, se requerir�a comunicaci�n Nios->HPS
            // Por ahora, esto solo resetea los contadores visuales locales.
            if (reset_request) {
                sample_count = 0;
                seconds_total = 0;
                display_time_4seg(SEG7_PTR, 0, 0);
                update_vga_timer_display(0);
                reset_request = 0;
            }

            /* C. Procesamiento de Audio */
            if (is_running) {
                if (audio_fifo_has_data()) {
                    // Llamamos a process_sample y verificamos si devuelve 0 (EOS)
                    int status = audio_process_sample();

                    if (status == 0) {
                        // Recibimos token de fin de canci�n
                        song_finished = 1; // Romper el while interno
                    }
                    else {
                        // Audio normal procesado, actualizar tiempo
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
                usleep(1000); // Pausa
            }
        } // Fin del while(!song_finished)

        printf("Cancion finalizada. Esperando siguiente...\n");
        vga_print_center("ESPERANDO SIGUIENTE...", 20);

    } // Fin del while(1) Playlist

    return 0;
}