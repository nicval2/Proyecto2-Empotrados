#include "player.h"
#include "buttons.h"
#include "switches.h"
#include "audio.h"
#include "ui.h"
#include "7_segments.h"
#include "system.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

/* Inicializa modulos del reproductor */
void player_init(void)
{
    buttons_init();
    filter_init();
    audio_init();
    switches_init();
    display_time_4seg(SEG7_PTR, 0, 0);
}

/* Espera handshake del HPS */
void player_wait_song(void)
{
    ui_show_waiting();
    audio_wait_handshake();
}

/* Reproduce una cancion hasta fin o salto */
void player_play_song(void)
{
    uint32_t sample_rate = audio_receive_metadata();
    uint32_t sample_count = 0;
    uint32_t seconds_total = 0;

    display_time_4seg(SEG7_PTR, 0, 0);
    ui_show_now_playing();

    int active = 1;

    while (active)
    {
        /* Actualiza ecualizador */
        switches_update();

        /* Reproduccion normal */
        if (is_running)
        {
            if (audio_fifo_has_data())
            {
                int status = audio_process_sample();

                /* 0 = fin, -1 = skip */
                if (status == 0 || status == -1)
                {
                    active = 0;
                }
                else
                {
                    sample_count++;

                    /* Un segundo de audio */
                    if (sample_count >= sample_rate)
                    {
                        sample_count = 0;
                        seconds_total++;

                        display_time_4seg(
                            SEG7_PTR,
                            seconds_total / 60,
                            seconds_total % 60
                        );

                        ui_update_timer(seconds_total);
                    }
                }
            }
        }
        else
        {
            /* Pausa */
            delay_ms(5);
        }
    }

    /* Cambio de pista */
    ui_transition();
    delay_ms(10);
}
