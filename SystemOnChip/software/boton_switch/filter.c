#include "filter.h"

// 8-tap FIR low-pass (cutoff ~3 kHz a 48 kHz sampling)
static const float lp_coeff[8] = {
    0.05, 0.10, 0.15, 0.20,
    0.20, 0.15, 0.10, 0.05
};

// buffer circular de muestras
static int16_t hist[8];
static int idx = 0;

static int lowpass_enabled = 0;

void filter_lowpass_enable() {
    lowpass_enabled = 1;
}

void filter_lowpass_disable() {
    lowpass_enabled = 0;
}

int16_t filter_lowpass_process(int16_t sample)
{
    if (!lowpass_enabled)
        return sample;  // sin filtrar

    hist[idx] = sample;

    float acc = 0;
    int p = idx;

    for(int i = 0; i < 8; i++) {
        acc += lp_coeff[i] * hist[p];
        p = (p - 1) & 0x07; // wrap 0..7
    }

    idx = (idx + 1) & 0x07;

    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;

    return (int16_t)acc;
}
