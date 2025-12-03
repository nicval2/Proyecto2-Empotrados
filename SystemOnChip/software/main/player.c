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

void player_init(void)
{
    buttons_init();
    filter_init();
    audio_init();
    switches_init();
    display_time_4seg(SEG7_PTR, 0, 0);
}

void player_wait_song(void)
{
    ui_show_waiting();
    audio_wait_handshake();
}

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
        switches_update();

        if (is_running)
        {
            if (audio_fifo_has_data())
            {
                int status = audio_process_sample();

                if (status == 0 || status == -1)
                {
                    active = 0;
                }
                else
                {
                    sample_count++;

                    if (sample_count >= sample_rate)
                    {
                        sample_count = 0;
                        seconds_total++;

                        display_time_4seg(SEG7_PTR,
                            seconds_total / 60,
                            seconds_total % 60);

                        ui_update_timer(seconds_total);
                    }
                }
            }
        }
        else
        {
            delay_ms(5);
        }
    }

    ui_transition();
    delay_ms(100);
}
