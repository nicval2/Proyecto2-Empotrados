#include <stdio.h>
#include "system.h"
#include "7_segments.h"
#include "buttons.h"
#include "switches.h"
#include "timer.h"
#include "vga.h"

#define SEG7_PTR  ((volatile int*)REG_7_SEGMENTS_BASE)

int main()
{
	alt_putstr("--- INICIANDO SOC ---\n");

    buttons_init();
    timer_init();

    int ms = 0, s = 0, m = 0;

    display_time_4seg(SEG7_PTR, 0, 0);

    vga_init();
	vga_clear();

	vga_print_center("Jigsaw falling into place", 9);
	vga_print_center("Radiohead", 10);
	vga_print_center("In rainbows", 11);

	char tiempo_str[6];
	sprintf(tiempo_str, "%02d:%02d", m, s);
	vga_print_center(tiempo_str, 12);




    while (1)
    {
        /* RESET solicitado por botones */
        if (reset_request)
        {
            ms = 0; s = 0; m = 0;
            display_time_4seg(SEG7_PTR, m, s);
            reset_request = 0;
        }

        /* TIMER */
        if (timer_tick())
        {
            if (is_running)
            {
                ms++;

                if (ms >= 1000)
                {
                    ms = 0;
                    s++;

                    if (s >= 60)
                    {
                        s = 0;
                        m++;
                    }

                    display_time_4seg(SEG7_PTR, m, s);

                    // === Actualizar texto VGA ===
                    vga_clear_line(12);
					sprintf(tiempo_str, "%02d:%02d", m, s);
					vga_print_center(tiempo_str, 12);
                }
            }
        }

        /* SWITCHES */
        switches_poll();
    }

    return 0;
}
