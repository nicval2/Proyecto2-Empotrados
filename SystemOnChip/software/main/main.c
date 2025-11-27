#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>
#include "filter.h"
#include "src/7_segment/7_segment.h"
#include "src/switches/switch.h"

int main()
{
    alt_putstr("=== AUDIO PLAYER SoC START ===\n");

    // Initialize modules
    buttons_init();
    switches_init();
    segment_timer_start();

    while (1)
    {
        // Update timer (MM:SS display)
        segment_timer_update();

        // Poll switches (filter selection)
        switches_update();
    }

    return 0;
}

