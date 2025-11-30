#include <stdio.h>
#include <unistd.h>
#include <string.h>      // Necesario para strlen, etc
#include "system.h"
#include "altera_avalon_pio_regs.h"

/* Mis m�dulos */
#include "7_segments.h"
#include "buttons.h"
#include "switches.h"
#include "timer.h"
#include "filter.h"
#include "audio.h"
#include "vga.h"         // <--- Agregamos VGA

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

/* Helper para actualizar el tiempo en VGA sin borrar todo */
void update_vga_timer_display(alt_u32 total_seconds) {
    char time_buf[16];
    // Formato MM:SS
    sprintf(time_buf, "%02ld:%02ld", total_seconds / 60, total_seconds % 60);
    // Imprimir centrado en la fila 15 (aprox mitad de pantalla)
    vga_print_center(time_buf, 15);
}

int main()
{
    alt_putstr("\n--- SISTEMA SOC REPRODUCTOR ---\n");

    // 1. Inicializar todo
    buttons_init();
    timer_init();
    filter_init();
    audio_init();
    vga_init();         // <--- Init VGA

    display_time_4seg(SEG7_PTR, 0, 0);

    // Pantalla de Bienvenida
    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 5);
    vga_print_center("Esperando conexion...", 7);

    // 2. Protocolo de Inicio de Audio
    audio_wait_handshake();

    // Obtenemos el sample rate
    alt_u32 sample_rate = audio_receive_metadata();

    printf("\n>>> REPRODUCIENDO <<<\n");
    printf("Titulo:  %s\n", title_str); // Variables vienen de audio.h
    printf("Artista: %s\n", artist_str);
    printf("Album:   %s\n", album_str);

    // --- ACTUALIZAR VGA CON METADATA ---
    vga_clear();
    vga_print_center("=== NOW PLAYING ===", 2);

    // Fila 5: T�tulo
    vga_print_center(title_str, 5);
    // Fila 7: Artista
    vga_print_center(artist_str, 7);
    // Fila 9: Album
    vga_print_center(album_str, 9);

    // Fila 13: Etiqueta de tiempo
    vga_print_center("TIME", 13);
    update_vga_timer_display(0);


    // Variables de tiempo
    alt_u32 sample_count = 0;
    alt_u32 seconds_total = 0;

    // 3. Bucle Infinito
    while (1)
    {
        /* A. LEER SWITCHES */
        int sw_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

        if(sw_val & 0x01) filter_set(FILTER_LOWPASS);
        else if(sw_val & 0x02) filter_set(FILTER_HIGHPASS);
        else filter_set(FILTER_NONE);

        if (sw_val & 0x200) volume_shift = 2;
        else volume_shift = 0;

        /* B. BOTONES */
        if (reset_request) {
            sample_count = 0;
            seconds_total = 0;
            display_time_4seg(SEG7_PTR, 0, 0);
            update_vga_timer_display(0); // Reset VGA time
            reset_request = 0;
        }

        /* C. PROCESAR AUDIO */
        if (is_running) {
            if (audio_fifo_has_data()) {
                audio_process_sample();

                // Actualizar Tiempo
                sample_count++;
                if(sample_count >= sample_rate) {
                    seconds_total++;
                    sample_count = 0;

                    // Hardware
                    display_time_4seg(SEG7_PTR, seconds_total / 60, seconds_total % 60);

                    // VGA (Actualizar solo la hora)
                    update_vga_timer_display(seconds_total);

                    // Consola
                    printf("\rTime: %02ld:%02ld\n", seconds_total / 60, seconds_total % 60);
                }
            }
        }
        else {
            usleep(1000);
        }
    }

    return 0;
}
