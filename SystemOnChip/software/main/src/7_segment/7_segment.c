#include "7_segment.h"
#include "sys/alt_stdio.h"
#include "system.h"

// 7-segment HEX table
static int seg7_table[10] = {
    0x40, 0x79, 0x24, 0x30, 0x19,
    0x12, 0x02, 0x78, 0x00, 0x10
};

static volatile unsigned int *timer_status_ptr;
static volatile unsigned int *timer_ctrl_ptr;
static volatile unsigned int *segments7_ptr;

// Time counters
static int elapsed_ms = 0;
static int elapsed_s  = 0;
static int elapsed_m  = 0;


// ------------------------------------------------------
// Write MM:SS to the four 7-segment displays
// ------------------------------------------------------
static void display_time_4seg(int minutes, int seconds)
{
    int m1 = (minutes / 10) % 10;
    int m0 = minutes % 10;

    int s1 = (seconds / 10) % 10;
    int s0 = seconds % 10;

    int hex3 = seg7_table[m1];
    int hex2 = seg7_table[m0];
    int hex1 = seg7_table[s1];
    int hex0 = seg7_table[s0];

    *segments7_ptr = (hex3 << 24) | (hex2 << 16) | (hex1 << 8) | hex0;
}


// ------------------------------------------------------
// Initialize timer and 7-segment display
// ------------------------------------------------------
void segment_timer_start()
{
    timer_status_ptr = (unsigned int *) TIMER_BASE;
    timer_ctrl_ptr   = timer_status_ptr + 1;
    segments7_ptr    = (unsigned int *) SEGMENTS7_BASE;

    alt_putstr("[TIMER] Starting timer...\n");

    if (*timer_status_ptr != 0) {
        alt_printf("[TIMER] ERROR: status != 0 (%x)\n", *timer_status_ptr);
        return;
    }

    // Enable timer: START=1, CONT=1
    *timer_ctrl_ptr = 0x6;

    // Wait until timer is running
    while (*timer_status_ptr != 0x2);

    alt_putstr("[TIMER] Timer running.\n");

    display_time_4seg(0, 0);
}


// ------------------------------------------------------
// Update timer and display when tick occurs
// ------------------------------------------------------
void segment_timer_update()
{
    if (*timer_status_ptr == 0x3) {

        elapsed_ms++;
        *timer_status_ptr = 0;   // Clear interrupt flag

        if (elapsed_ms >= 1000) {
            elapsed_ms = 0;
            elapsed_s++;
        }

        if (elapsed_s >= 60) {
            elapsed_s = 0;
            elapsed_m++;
        }

        display_time_4seg(elapsed_m, elapsed_s);
    }
}
