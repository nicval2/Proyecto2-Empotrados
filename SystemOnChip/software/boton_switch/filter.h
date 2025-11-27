#ifndef FILTER_H_
#define FILTER_H_

#include <stdint.h>

void filter_lowpass_enable();
void filter_lowpass_disable();
int16_t filter_lowpass_process(int16_t sample);

#endif
