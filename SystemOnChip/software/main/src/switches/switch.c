#include "switch.h"
#include "system.h"
#include "sys/alt_stdio.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <unistd.h>
#include "filter.h"

// Button bit masks
#define BUTTON_PLAY_MASK  0x8
#define BUTTON_NEXT_MASK  0x4
#define BUTTON_PREV_MASK  0x2

// Internal variables
static volatile int edge_capture = 0;
static int last_switch_value = -1;


// ======================================================
// Button interrupt service routine
// ======================================================
static void handle_button_interrupts(void* context)
{
    volatile int value;
    value = IORD_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE);

    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, value);

    if (value & BUTTON_PLAY_MASK) {
        alt_putstr("[BUTTON] PLAY\n");
    }
    else if (value & BUTTON_NEXT_MASK) {
        alt_putstr("[BUTTON] NEXT\n");
    }
    else if (value & BUTTON_PREV_MASK) {
        alt_putstr("[BUTTON] PREV\n");
    }
}


// ======================================================
// Initialize button PIO and IRQ
// ======================================================
void buttons_init()
{
    IOWR_ALTERA_AVALON_PIO_EDGE_CAP(REG_BUTTONS_BASE, 0x00);

    alt_ic_isr_register(
        REG_BUTTONS_IRQ_INTERRUPT_CONTROLLER_ID,
        REG_BUTTONS_IRQ,
        handle_button_interrupts,
        NULL,
        0x00
    );

    IOWR_ALTERA_AVALON_PIO_IRQ_MASK(REG_BUTTONS_BASE, 0x0E);

    alt_putstr("[BUTTONS] Ready\n");
}


// ======================================================
// Initialize switches (polling mode, no IRQ)
// ======================================================
void switches_init()
{
    last_switch_value = -1;
    alt_putstr("[SWITCHES] Ready\n");
}


// ======================================================
// Poll switches and apply filter selection
// ======================================================
void switches_update()
{
    int value = IORD_ALTERA_AVALON_PIO_DATA(REG_SWITCHES_BASE);

    if (value != last_switch_value)
    {
        last_switch_value = value;

        alt_putstr("[SWITCH] New value: ");
        alt_printf("%x\n", value);

        if (value == 1)
        {
            filter_lowpass_enable();
            filter_highpass_disable();
            alt_putstr("[FILTER] Low-pass enabled\n");
        }
        else if (value == 2)
        {
            filter_highpass_enable();
            filter_lowpass_disable();
            alt_putstr("[FILTER] High-pass enabled\n");
        }
        else
        {
            filter_lowpass_disable();
            filter_highpass_disable();
            alt_putstr("[FILTER] All filters disabled\n");
        }
    }

    usleep(100000);
}


// ======================================================
// Getter for last switch value
// ======================================================
int switches_get_value()
{
    return last_switch_value;
}
