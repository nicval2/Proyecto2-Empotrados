#include "filter.h"

// ============================
//  LOW-PASS FIR 8 TAPS
// ============================

// 8-tap FIR low-pass (cutoff ~3 kHz @ 48 kHz sampling)
static const float lp_coeff[8] = {
    0.05, 0.10, 0.15, 0.20,
    0.20, 0.15, 0.10, 0.05
};

static int16_t lp_hist[8];
static int lp_idx = 0;
static int lowpass_enabled = 0;

void filter_lowpass_enable()  { lowpass_enabled = 1; }
void filter_lowpass_disable() { lowpass_enabled = 0; }


int16_t filter_lowpass_process(int16_t sample)
{
    if (!lowpass_enabled)
        return sample;

    lp_hist[lp_idx] = sample;
    float acc = 0;
    int p = lp_idx;

    for (int i = 0; i < 8; i++) {
        acc += lp_coeff[i] * lp_hist[p];
        p = (p - 1) & 0x07;
    }

    lp_idx = (lp_idx + 1) & 0x07;

    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;

    return (int16_t)acc;
}



// ============================
//  HIGH-PASS FIR 8 TAPS
// ============================

// Este FIR elimina bajas frecuencias y deja pasar los agudos.
// Diseñado para 48kHz, cutoff ~3-4kHz.
static const float hp_coeff[8] = {
   -0.05, -0.10, -0.15,  0.70,
    0.70, -0.15, -0.10, -0.05
};

static int16_t hp_hist[8];
static int hp_idx = 0;
static int highpass_enabled = 0;

void filter_highpass_enable()  { highpass_enabled = 1; }
void filter_highpass_disable() { highpass_enabled = 0; }

int16_t filter_highpass_process(int16_t sample)
{
    if (!highpass_enabled)
        return sample;

    hp_hist[hp_idx] = sample;
    float acc = 0;
    int p = hp_idx;

    for (int i = 0; i < 8; i++) {
        acc += hp_coeff[i] * hp_hist[p];
        p = (p - 1) & 0x07;
    }

    hp_idx = (hp_idx + 1) & 0x07;

    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;

    return (int16_t)acc;
}
