#ifndef SEGMENT_TIMER_H_
#define SEGMENT_TIMER_H_

#include <stdint.h>

#define SEGMENTS7_BASE 0x3000
#define TIMER_BASE     0x3020

void segment_timer_start();
void segment_timer_update();

#endif
