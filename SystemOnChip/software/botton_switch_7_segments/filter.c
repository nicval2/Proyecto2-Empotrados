#include "filter.h"

/*
 * Implementación de filtros IIR primero orden (muy rápidos)
 *
 * y[n] = a*y[n-1] + b*x[n]
 *
 * LOW PASS  (LPF): deja pasar bajas frecuencias
 * HIGH PASS (HPF): deja pasar altas frecuencias
 *
 * Las constantes están escaladas en Q15 (fijo entero)
 * para trabajar eficientemente en NIOS II.
 */

static filter_type_t current_filter = FILTER_NONE;

/* Estado interno */
static int32_t y_prev = 0;

/* Constantes Q15 */
#define Q15(x) ((int32_t)((x) * 32768.0f))

/* Coeficientes del LPF (frecuencia de corte ~2kHz si fs = 48kHz) */
static const int32_t LPF_A = Q15(0.85f);
static const int32_t LPF_B = Q15(0.15f);

/* Coeficientes del HPF */
static const int32_t HPF_A = Q15(-0.85f);
static const int32_t HPF_B = Q15( 0.85f);

void filter_init(void)
{
    current_filter = FILTER_NONE;
    y_prev = 0;
}

void filter_set(filter_type_t type)
{
    current_filter = type;
    y_prev = 0;
}

int16_t filter_process(int16_t x)
{
    int32_t xn = (int32_t)x;
    int32_t yn = 0;

    switch (current_filter)
    {
        case FILTER_LOWPASS:
            yn = ( (LPF_A * y_prev) >> 15 ) + ( (LPF_B * xn) >> 15 );
            break;

        case FILTER_HIGHPASS:
            yn = ( (HPF_A * y_prev) >> 15 ) + ( (HPF_B * xn) >> 15 );
            break;

        case FILTER_NONE:
        default:
            return x;   // Sin filtrar
    }

    /* Saturación dentro de rango de 16 bits */
    if (yn > 32767) yn = 32767;
    if (yn < -32768) yn = -32768;

    y_prev = yn;
    return (int16_t)yn;
}
