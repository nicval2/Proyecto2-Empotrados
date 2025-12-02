#include "timer.h"
#include "system.h"
#include "altera_avalon_timer_regs.h"

/*
 * Configura: START | CONT = 0x6
 */
void timer_init(void)
{
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x6);
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
}

int timer_tick(void)
{
    int st = IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE);

    if (st & 1)
    {
        IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
        return 1;
    }
    return 0;
}
