#include "7_segments.h"

static int seg7_table[10] = {
    0x40, 0x79, 0x24, 0x30, 0x19,
    0x12, 0x02, 0x78, 0x00, 0x10
};

void display_time_4seg(volatile int *hex_ptr, int minutes, int seconds)
{
    int m1 = (minutes / 10) % 10;
    int m0 = minutes % 10;
    int s1 = (seconds / 10) % 10;
    int s0 = seconds % 10;

    int hex3 = seg7_table[m1];
    int hex2 = seg7_table[m0];
    int hex1 = seg7_table[s1];
    int hex0 = seg7_table[s0];

    *hex_ptr = (hex3 << 24) | (hex2 << 16) | (hex1 << 8) | hex0;
}
