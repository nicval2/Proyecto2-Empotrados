#include "timer.h"
#include "system.h"
#include <stdint.h>

/* Registros del timer */
#define TIMER_STATUS     (*(volatile uint32_t*)(TIMER_BASE + 0x00))
#define TIMER_CONTROL    (*(volatile uint32_t*)(TIMER_BASE + 0x04))
#define TIMER_PERIODL    (*(volatile uint32_t*)(TIMER_BASE + 0x08))
#define TIMER_PERIODH    (*(volatile uint32_t*)(TIMER_BASE + 0x0C))

/*
 * CONTROL BITS:
 * 0 = STOP
 * 1 = START
 * 2 = CONT
 * 3 = ITO (interrupt enable)
 */

void timer_init(void)
{

    TIMER_CONTROL = 0x6;

    /* Limpiar el flag de timeout */
    TIMER_STATUS = 0;
}

int timer_tick(void)
{
    uint32_t status = TIMER_STATUS;

    /* Bit 0 = TO (timeout) */
    if (status & 1)
    {
        /* Limpiar timeout */
        TIMER_STATUS = 0;
        return 1;
    }
    return 0;
}
