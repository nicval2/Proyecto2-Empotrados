// filter.c - Ecualizador con filtros Biquad IIR
#include "filter.h"
#include <string.h>

/*
 * Implementación de filtros Biquad (IIR segundo orden)
 *
 * Ecuación: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 *
 * Los coeficientes están en formato Q15 (escalados por 32768)
 * para aritmética de punto fijo eficiente en NIOS II.
 *
 * Frecuencia de muestreo asumida: ~48-76 kHz
 */

/* Escala Q15 */
#define Q15_SCALE 32768
#define Q15(x) ((int32_t)((x) * Q15_SCALE))

/* Estructura de coeficientes biquad */
typedef struct {
    int32_t b0, b1, b2;  // Numerador
    int32_t a1, a2;      // Denominador (a0 = 1.0 implícito)
} biquad_coeffs_t;

/* Estado del filtro (memoria) */
typedef struct {
    int32_t x1, x2;  // Entradas anteriores
    int32_t y1, y2;  // Salidas anteriores
} biquad_state_t;

/* Variables globales */
static filter_type_t current_filter = FILTER_NONE;
static biquad_state_t state1 = {0};  // Primera etapa
static biquad_state_t state2 = {0};  // Segunda etapa (para filtros complejos)
static int use_second_stage = 0;

/*
 * Coeficientes pre-calculados para cada filtro
 * Calculados para fs = 48kHz (funcionan similar para otras frecuencias)
 */

/* Sin filtro (bypass) */
static const biquad_coeffs_t coeff_bypass = {
    .b0 = Q15(1.0), .b1 = Q15(0.0), .b2 = Q15(0.0),
    .a1 = Q15(0.0), .a2 = Q15(0.0)
};

/* Bass Boost: Low-shelf filter, +6dB @ 200Hz */
static const biquad_coeffs_t coeff_bass_boost = {
    .b0 = Q15(1.0636),  .b1 = Q15(-1.9124), .b2 = Q15(0.8577),
    .a1 = Q15(-1.9219), .a2 = Q15(0.9306)
};

/* Bass Cut: Low-shelf filter, -6dB @ 200Hz */
static const biquad_coeffs_t coeff_bass_cut = {
    .b0 = Q15(0.9403),  .b1 = Q15(-1.7982), .b2 = Q15(0.8649),
    .a1 = Q15(-1.8052), .a2 = Q15(0.8122)
};

/* Treble Boost: High-shelf filter, +6dB @ 4kHz */
static const biquad_coeffs_t coeff_treble_boost = {
    .b0 = Q15(1.2247),  .b1 = Q15(-1.5691), .b2 = Q15(0.6199),
    .a1 = Q15(-1.3897), .a2 = Q15(0.5653)
};

/* Treble Cut: High-shelf filter, -6dB @ 4kHz */
static const biquad_coeffs_t coeff_treble_cut = {
    .b0 = Q15(0.8167),  .b1 = Q15(-1.1348), .b2 = Q15(0.4614),
    .a1 = Q15(-1.2771), .a2 = Q15(0.4203)
};

/* Vocal/Mid Boost: Peaking filter, +4dB @ 2kHz, Q=1.0 */
static const biquad_coeffs_t coeff_vocal = {
    .b0 = Q15(1.0691),  .b1 = Q15(-1.4542), .b2 = Q15(0.5765),
    .a1 = Q15(-1.4542), .a2 = Q15(0.6456)
};

/* Low-pass @ 3kHz (técnico) */
static const biquad_coeffs_t coeff_lowpass = {
    .b0 = Q15(0.0976),  .b1 = Q15(0.1953), .b2 = Q15(0.0976),
    .a1 = Q15(-0.9428), .a2 = Q15(0.3333)
};

/* High-pass @ 300Hz (técnico) */
static const biquad_coeffs_t coeff_highpass = {
    .b0 = Q15(0.9391),  .b1 = Q15(-1.8782), .b2 = Q15(0.9391),
    .a1 = Q15(-1.8745), .a2 = Q15(0.8819)
};

/* Punteros a coeficientes activos */
static const biquad_coeffs_t *active_coeffs1 = &coeff_bypass;
static const biquad_coeffs_t *active_coeffs2 = &coeff_bypass;

/* Nombres de filtros para mostrar en VGA */
static const char* filter_names[] = {
    "NORMAL",
    "BASS+",
    "BASS-",
    "TREBLE+",
    "TREBLE-",
    "VOCAL",
    "ROCK",
    "POP",
    "JAZZ",
    "LOW-PASS",
    "HIGH-PASS"
};

/* Función de procesamiento biquad */
static int32_t biquad_process(const biquad_coeffs_t *c, biquad_state_t *s, int32_t x) {
    int64_t y;

    /* Calcular salida con 64 bits para evitar overflow */
    y = (int64_t)c->b0 * x;
    y += (int64_t)c->b1 * s->x1;
    y += (int64_t)c->b2 * s->x2;
    y -= (int64_t)c->a1 * s->y1;
    y -= (int64_t)c->a2 * s->y2;

    /* Escalar de vuelta a Q15 */
    y = y >> 15;

    /* Actualizar estado */
    s->x2 = s->x1;
    s->x1 = x;
    s->y2 = s->y1;
    s->y1 = (int32_t)y;

    return (int32_t)y;
}

/* Reset del estado de los filtros */
static void reset_state(void) {
    memset(&state1, 0, sizeof(state1));
    memset(&state2, 0, sizeof(state2));
}

/* Funciones públicas */

void filter_init(void) {
    current_filter = FILTER_NONE;
    active_coeffs1 = &coeff_bypass;
    active_coeffs2 = &coeff_bypass;
    use_second_stage = 0;
    reset_state();
}

void filter_set(filter_type_t type) {
    if (type == current_filter) return;

    current_filter = type;
    use_second_stage = 0;

    switch (type) {
        case FILTER_NONE:
            active_coeffs1 = &coeff_bypass;
            break;

        case FILTER_BASS_BOOST:
            active_coeffs1 = &coeff_bass_boost;
            break;

        case FILTER_BASS_CUT:
            active_coeffs1 = &coeff_bass_cut;
            break;

        case FILTER_TREBLE_BOOST:
            active_coeffs1 = &coeff_treble_boost;
            break;

        case FILTER_TREBLE_CUT:
            active_coeffs1 = &coeff_treble_cut;
            break;

        case FILTER_VOCAL:
            active_coeffs1 = &coeff_vocal;
            break;

        case FILTER_ROCK:
            /* Rock: Bass boost + Treble boost (V-shape) */
            active_coeffs1 = &coeff_bass_boost;
            active_coeffs2 = &coeff_treble_boost;
            use_second_stage = 1;
            break;

        case FILTER_POP:
            /* Pop: Vocal boost + slight treble */
            active_coeffs1 = &coeff_vocal;
            active_coeffs2 = &coeff_treble_boost;
            use_second_stage = 1;
            break;

        case FILTER_JAZZ:
            /* Jazz: Slight bass boost + treble cut (warm) */
            active_coeffs1 = &coeff_bass_boost;
            active_coeffs2 = &coeff_treble_cut;
            use_second_stage = 1;
            break;

        case FILTER_LOWPASS:
            active_coeffs1 = &coeff_lowpass;
            break;

        case FILTER_HIGHPASS:
            active_coeffs1 = &coeff_highpass;
            break;

        default:
            active_coeffs1 = &coeff_bypass;
            break;
    }

    /* Reset estado al cambiar filtro para evitar glitches */
    reset_state();
}

filter_type_t filter_get(void) {
    return current_filter;
}

const char* filter_get_name(void) {
    if (current_filter < sizeof(filter_names)/sizeof(filter_names[0])) {
        return filter_names[current_filter];
    }
    return "UNKNOWN";
}

int16_t filter_process(int16_t sample) {
    int32_t x = (int32_t)sample;
    int32_t y;

    /* Primera etapa */
    y = biquad_process(active_coeffs1, &state1, x);

    /* Segunda etapa (si está activa) */
    if (use_second_stage) {
        y = biquad_process(active_coeffs2, &state2, y);
    }

    /* Saturación a 16 bits */
    if (y > 32767) y = 32767;
    if (y < -32768) y = -32768;

    return (int16_t)y;
}

int filter_update_from_switches(int sw_value) {
    filter_type_t new_filter = FILTER_NONE;
    static filter_type_t last_filter = FILTER_NONE;

    /* Decodificar switches (prioridad de menor a mayor) */
    /* Solo un filtro activo a la vez, el de mayor prioridad gana */

    if (sw_value & 0x80)      new_filter = FILTER_JAZZ;        // SW7
    else if (sw_value & 0x40) new_filter = FILTER_POP;         // SW6
    else if (sw_value & 0x20) new_filter = FILTER_ROCK;        // SW5
    else if (sw_value & 0x10) new_filter = FILTER_VOCAL;       // SW4
    else if (sw_value & 0x08) new_filter = FILTER_TREBLE_CUT;  // SW3
    else if (sw_value & 0x04) new_filter = FILTER_TREBLE_BOOST;// SW2
    else if (sw_value & 0x02) new_filter = FILTER_BASS_CUT;    // SW1
    else if (sw_value & 0x01) new_filter = FILTER_BASS_BOOST;  // SW0
    else new_filter = FILTER_NONE;

    if (new_filter != last_filter) {
        filter_set(new_filter);
        last_filter = new_filter;
        return 1;  /* Filtro cambió */
    }

    return 0;  /* Sin cambio */
}
