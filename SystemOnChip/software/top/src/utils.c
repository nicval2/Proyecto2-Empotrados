#include "utils.h"

static void delay_cycles(uint32_t cycles)
{
    while (cycles--) asm volatile("nop");
}

void delay_ms(uint32_t ms)
{
    delay_cycles(ms * CLOCK);
}
