#include <stdio.h>
#include "player.h"
#include "ui.h"
#include "utils.h"
#include "audio.h"

int main()
{
    printf("\n=== REPRODUCTOR NIOS v2.3 ===\n");

    ui_init();
    player_init();

    delay_ms(100);
    audio_send_ready_signal();

    while (1)
    {
        player_wait_song();
        player_play_song();
    }

    return 0;
}
