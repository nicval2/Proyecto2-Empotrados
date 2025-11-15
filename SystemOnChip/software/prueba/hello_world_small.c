#include "sys/alt_stdio.h"
#include "system.h"
#include "sys/alt_irq.h"

#define LEDS_PTR        ((volatile unsigned int*) REG_LEDS_BASE)
#define BUTTON_PTR      ((volatile unsigned int*) REG_BUTTON_BASE)
#define TIMER_STATUS    ((volatile unsigned int*) TIMER_BASE)
#define TIMER_CONTROL   ((volatile unsigned int*) (TIMER_BASE + 4))

volatile int elapsed_ms = 0;

static void timer_isr(void* context, alt_u32 id)
{
    *TIMER_STATUS = 0;
    elapsed_ms++;
    alt_printf("IRQ %x\n", elapsed_ms);
}

int main()
{
    alt_putstr("Hello from Nios II with IRQ!\n");

    if (*TIMER_STATUS != 0) {
        alt_printf("ERR %x\n", *TIMER_STATUS);
        return 0;
    }

    *TIMER_CONTROL = 0x7;

    alt_ic_isr_register(
        TIMER_IRQ_INTERRUPT_CONTROLLER_ID,
        TIMER_IRQ,
        timer_isr,
        NULL,
        0
    );

    alt_putstr("Timer running...\n");

    while (1)
    {
        *LEDS_PTR = elapsed_ms;
    }

    return 0;
}
