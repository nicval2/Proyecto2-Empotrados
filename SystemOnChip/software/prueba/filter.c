#include "filter.h"

/*
 * Filtros IIR de primer orden - simples y estables
 *
 * Low-pass:  y[n] = alpha * x[n] + (1-alpha) * y[n-1]
 * High-pass: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
 *
 * Shelf filters usando combinaci�n de LP/HP con ganancia
 */

/* Estado del filtro */
static filter_type_t current_filter = FILTER_NONE;
static int32_t y_prev = 0;
static int32_t x_prev = 0;
static int32_t y2_prev = 0;
static int32_t x2_prev = 0;

/* Nombres de filtros */
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

/* Funciones de filtro individuales */

/* Low-pass filter: deja pasar graves */
static int16_t lowpass(int16_t x, int32_t *y_state, int alpha_shift) {
    /* alpha = 1 / (2^alpha_shift), t�picamente 2-4 */
    /* y = y_prev + alpha * (x - y_prev) */
    int32_t xn = (int32_t)x;
    int32_t yn = *y_state + ((xn - *y_state) >> alpha_shift);
    *y_state = yn;
    return (int16_t)yn;
}

/* High-pass filter: deja pasar agudos */
static int16_t highpass(int16_t x, int32_t *y_state, int32_t *x_state, int alpha_shift) {
    /* y = alpha * (y_prev + x - x_prev) */
    int32_t xn = (int32_t)x;
    int32_t diff = xn - *x_state;
    int32_t yn = ((*y_state + diff) * ((1 << alpha_shift) - 1)) >> alpha_shift;
    *x_state = xn;
    *y_state = yn;
    return (int16_t)yn;
}

/* Reset estado */
static void reset_state(void) {
    y_prev = 0;
    x_prev = 0;
    y2_prev = 0;
    x2_prev = 0;
}

/* Funciones p�blicas */

void filter_init(void) {
    current_filter = FILTER_NONE;
    reset_state();
}

void filter_set(filter_type_t type) {
    if (type == current_filter) return;
    current_filter = type;
    reset_state();
}

filter_type_t filter_get(void) {
    return current_filter;
}

const char* filter_get_name(void) {
    if (current_filter < sizeof(filter_names)/sizeof(filter_names[0])) {
        return filter_names[current_filter];
    }
    return "???";
}

int16_t filter_process(int16_t x) {
    int32_t xn = (int32_t)x;
    int32_t yn = xn;
    int32_t low, high;

    switch (current_filter) {
        case FILTER_NONE:
            /* Sin filtro - pasar directo */
            return x;

        case FILTER_BASS_BOOST:
            /* Extraer graves y amplificarlos */
            low = lowpass(x, &y_prev, 3);  /* fc ~ fs/16 */
            yn = xn + (low >> 1);  /* +50% graves */
            break;

        case FILTER_BASS_CUT:
            /* Reducir graves */
            low = lowpass(x, &y_prev, 3);
            yn = xn - (low >> 1);  /* -50% graves */
            break;

        case FILTER_TREBLE_BOOST:
            /* Extraer agudos y amplificarlos */
            high = highpass(x, &y_prev, &x_prev, 2);  /* fc ~ fs/8 */
            yn = xn + (high >> 1);  /* +50% agudos */
            break;

        case FILTER_TREBLE_CUT:
            /* Reducir agudos (equivale a lowpass) */
            yn = lowpass(x, &y_prev, 2);
            yn = (xn + yn) >> 1;  /* Mezcla 50/50 */
            break;

        case FILTER_VOCAL:
            /* Realzar medios: atenuar graves y agudos */
            low = lowpass(x, &y_prev, 4);
            high = highpass(x, &y2_prev, &x2_prev, 3);
            /* Medios = original - graves - agudos */
            yn = xn + ((xn - low - high) >> 2);  /* +25% medios */
            break;

        case FILTER_ROCK:
            /* V-shape: boost graves y agudos */
            low = lowpass(x, &y_prev, 3);
            high = highpass(x, &y2_prev, &x2_prev, 2);
            yn = xn + (low >> 2) + (high >> 2);  /* +25% cada uno */
            break;

        case FILTER_POP:
            /* Medios y agudos */
            high = highpass(x, &y_prev, &x_prev, 3);
            yn = xn + (high >> 2);  /* +25% agudos */
            break;

        case FILTER_JAZZ:
            /* Graves suaves, sonido c�lido */
            low = lowpass(x, &y_prev, 3);
            yn = xn + (low >> 2);  /* +25% graves */
            /* Luego suavizar agudos */
            yn = (yn + (int32_t)lowpass((int16_t)yn, &y2_prev, 2)) >> 1;
            break;

        case FILTER_LOWPASS:
            /* Filtro t�cnico: solo graves */
            yn = lowpass(x, &y_prev, 2);
            break;

        case FILTER_HIGHPASS:
            /* Filtro t�cnico: solo agudos */
            yn = highpass(x, &y_prev, &x_prev, 2);
            break;

        default:
            return x;
    }

    /* Saturaci�n */
    if (yn > 32767) yn = 32767;
    if (yn < -32768) yn = -32768;

    return (int16_t)yn;
}

int filter_update_from_switches(int sw_value) {
    filter_type_t new_filter = FILTER_NONE;
    static filter_type_t last_filter = FILTER_NONE;

    /* Decodificar switches - prioridad al m�s alto */
    if (sw_value & 0x80)      new_filter = FILTER_JAZZ;
    else if (sw_value & 0x40) new_filter = FILTER_POP;
    else if (sw_value & 0x20) new_filter = FILTER_ROCK;
    else if (sw_value & 0x10) new_filter = FILTER_VOCAL;
    else if (sw_value & 0x08) new_filter = FILTER_TREBLE_CUT;
    else if (sw_value & 0x04) new_filter = FILTER_TREBLE_BOOST;
    else if (sw_value & 0x02) new_filter = FILTER_BASS_CUT;
    else if (sw_value & 0x01) new_filter = FILTER_BASS_BOOST;

    if (new_filter != last_filter) {
        filter_set(new_filter);
        last_filter = new_filter;
        return 1;
    }

    return 0;
}
