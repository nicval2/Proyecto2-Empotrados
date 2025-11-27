#ifndef FILTER_H_
#define FILTER_H_

#include <stdint.h>

// ---- LOW PASS ----
void filter_lowpass_enable();
void filter_lowpass_disable();
int16_t filter_lowpass_process(int16_t sample);

// ---- HIGH PASS ----
void filter_highpass_enable();
void filter_highpass_disable();
int16_t filter_highpass_process(int16_t sample);

#endif
