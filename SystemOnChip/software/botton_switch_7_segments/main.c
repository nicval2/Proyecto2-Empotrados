    #include <stdio.h>
    #include <unistd.h>
    #include <string.h>
    #include "system.h"
    #include "altera_avalon_pio_regs.h"

    /* M�dulos */
    #include "7_segments.h"
    #include "buttons.h"
    #include "switches.h"
    #include "timer.h"
    #include "filter.h"
    #include "audio.h"
    #include "vga.h"

    #define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

    /* Helper para actualizar tiempo en VGA */
    void update_vga_timer_display(alt_u32 total_seconds) {
        char time_buf[16];
        sprintf(time_buf, "%02ld:%02ld", total_seconds / 60, total_seconds % 60);
        // Imprimir centrado en fila 15
        vga_print_center(time_buf, 15);
    }

    int main()
    {
        alt_putstr("\n--- SISTEMA SOC REPRODUCTOR ---\n");

        // 1. Inicializar Hardware
        buttons_init();
        timer_init();
        filter_init();
        audio_init();
        vga_init();

        // Pantalla Bienvenida
        vga_clear();
        vga_print_center("=== FPGA PLAYER ===", 5);
        vga_print_center("Waiting for ARM...", 7);

        // =========================================================
        // BUCLE DE PLAYLIST (Carga canci�n tras canci�n)
        // =========================================================
        while(1)
        {
            // 2. Esperar Nueva Cancin (Handshake)
            printf("\nWaiting for song...\n");

            // Si venimos de un Reset (Botn Next), limpiamos flags
            reset_request = 0;

            // Esta funci�n se bloquea hasta recibir el MAGIC NUMBER del ARM
            audio_wait_handshake();

            // 3. Recibir Metadata Nueva
            alt_u32 sample_rate = audio_receive_metadata();

            // 4. Actualizar Interfaz (Consola + VGA + 7Seg)
            printf("\n>>> REPRODUCIENDO <<<\n");
            printf("Titulo:  %s\n", title_str);
            printf("Artista: %s\n", artist_str);

            vga_clear();
            vga_print_center("=== NOW PLAYING ===", 2);
            vga_print_center(title_str, 5);
            vga_print_center(artist_str, 7);
            vga_print_center(album_str, 9);
            vga_print_center("TIME", 13);

            alt_u32 sample_count = 0;
            alt_u32 seconds_total = 0;

            display_time_4seg(SEG7_PTR, 0, 0);
            update_vga_timer_display(0);

            // =====================================================
            // BUCLE DE REPRODUCCI�N (Samples de audio)
            // =====================================================
            int song_finished = 0;
            int timeout_counter = 0;

            while (!song_finished)
            {
                // A. Revisar Switches (Filtros/Volumen)
                int sw_val = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

                if(sw_val & 0x01) filter_set(FILTER_LOWPASS);
                else if(sw_val & 0x02) filter_set(FILTER_HIGHPASS);
                else filter_set(FILTER_NONE);

                if (sw_val & 0x200) volume_shift = 2;
                else volume_shift = 0;

                // B. Revisar Botones (Next/Prev)
                if (reset_request) {
                    // El usuario presion� un bot�n. Buttons.c ya envi� el comando al ARM.
                    // Salimos de este bucle para esperar la metadata de la nueva canci�n.
                    song_finished = 1;
                    break;
                }

                // C. Procesar Audio
                if (is_running) {
                    if (audio_fifo_has_data()) {
                        audio_process_sample();
                        timeout_counter = 0; // Reset timeout

                        // Tiempo
                        sample_count++;
                        if(sample_count >= sample_rate) {
                            seconds_total++;
                            sample_count = 0;
                            display_time_4seg(SEG7_PTR, seconds_total / 60, seconds_total % 60);
                            update_vga_timer_display(seconds_total);
                            printf("\rTime: %02ld:%02ld", seconds_total / 60, seconds_total % 60);
                        }
                    }
                    else {
                        // FIFO Vac�a. �Fin de cancin?
                        // Esperamos un poco. Si sigue vac�a mucho tiempo, asumimos que acab�.
                        usleep(100);
                        timeout_counter++;
                        if (timeout_counter > 50000) { // ~5 segundos sin datos
                            // Opcional: Auto-detectar fin.
                            // Pero mejor dejemos que el ARM mande la siguiente o espere bot�n.
                            timeout_counter = 0;
                        }
                    }
                } else {
                    usleep(1000); // Pausa
                }
            }

            printf("\nSong ended or changed.\n");
        }

        return 0;
    }
