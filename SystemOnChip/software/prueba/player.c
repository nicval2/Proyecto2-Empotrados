#include "player.h"
#include "system.h"
#include "audio.h"
#include "vga.h"
#include "buttons.h"
#include "filter.h"
#include "7_segments.h"

/* --- Definiciones de Hardware --- */
#define SWITCHES_PTR  ((volatile int *)REG_SWITCHES_BASE)
#define SEG7_PTR      ((volatile int *)REG_7_SEGMENTS_BASE)

/* --- Utilidades Privadas (Inline para optimización) --- */

/* Retardo por software para evitar dependencias de drivers de timer */
static void delay_soft(int loops) {
    volatile int i;
    for(i = 0; i < loops; i++);
}

/* Conversión int a string ligera (evita sprintf que pesa mucho) */
static void time_to_str(char *buf, unsigned int seconds) {
    unsigned int min = seconds / 60;
    unsigned int sec = seconds % 60;

    buf[0] = (min / 10) + '0';
    buf[1] = (min % 10) + '0';
    buf[2] = ':';
    buf[3] = (sec / 10) + '0';
    buf[4] = (sec % 10) + '0';
    buf[5] = '\0';
}

/* --- Funciones de Interfaz Gráfica (VGA) --- */

static void ui_update_timer(unsigned int seconds) {
    char buf[8];
    time_to_str(buf, seconds);
    vga_print_center(buf, 15);
}

static void ui_update_filter(void) {
    vga_clear_line(17);
    vga_print_center(filter_get_name(), 17);
}

static void ui_show_waiting(void) {
    vga_clear();
    vga_print_center("=== REPRODUCTOR FPGA ===", 3);
    vga_print_center("Esperando HPS...", 7);
    vga_print_center("SW0-7: Filtros | KEY3: Play", 20);
}

static void ui_show_playing(void) {
    vga_clear();
    vga_print_center("=== REPRODUCIENDO ===", 2);
    vga_print_center(title_str, 5);  // Variables extern de audio.h
    vga_print_center(artist_str, 7);
    vga_print_center(album_str, 9);

    vga_print_center("TIEMPO:", 13);
    ui_update_timer(0);

    vga_print_center("FILTRO ACTUAL:", 16);
    ui_update_filter();

    vga_print_center("[<<]      [ >/|| ]      [>>]", 25);
}

/* --- Implementación Pública --- */

void player_init(void) {
    buttons_init();
    filter_init();
    audio_init();
    vga_init();

    // Estado inicial visual
    display_time_4seg(SEG7_PTR, 0, 0);
    ui_show_waiting();

    delay_soft(100000); // Estabilización
    audio_send_ready_signal();
}

void player_loop(void) {
    while(1) {
        /* 1. Preparación para nueva canción */
        unsigned int sample_count = 0;
        unsigned int seconds_total = 0;
        int song_active = 1;
        int last_sw = -1;

        display_time_4seg(SEG7_PTR, 0, 0);
        buttons_clear_skip();

        /* 2. Sincronización con HPS */
        vga_clear();
        vga_print_center("Cargando...", 10);

        audio_wait_handshake();
        unsigned int sample_rate = audio_receive_metadata();

        /* 3. Inicia reproducción */
        ui_show_playing();

        while (song_active) {
            /* A. Gestión de Ecualizador (Polling de Switches) */
            int sw = *SWITCHES_PTR;
            if (sw != last_sw) {
                if (filter_update_from_switches(sw & 0xFF)) {
                    ui_update_filter();
                }
                // Bit 9 controla volumen (Shift bit)
                volume_shift = (sw & 0x200) ? 2 : 0;
                last_sw = sw;
            }

            /* B. Procesamiento de Audio */
            if (is_running) { // Variable de buttons.h
                if (audio_fifo_has_data()) {
                    int status = audio_process_sample();

                    if (status == 0) {
                        song_active = 0; // Fin de canción (EOS)
                    }
                    else if (status == -1) {
                        song_active = 0; // Salto de canción (SKIP)
                    }
                    else {
                        // Actualización de tiempo (sin float)
                        sample_count++;
                        if (sample_count >= sample_rate) {
                            seconds_total++;
                            sample_count = 0;
                            display_time_4seg(SEG7_PTR, seconds_total / 60, seconds_total % 60);
                            ui_update_timer(seconds_total);
                        }
                    }
                }
            } else {
                delay_soft(5000); // Pausa: bajo consumo relativo
            }
        }

        delay_soft(50000); // Debounce entre canciones
    }
}
