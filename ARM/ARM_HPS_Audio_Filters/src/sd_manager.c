// ----------------------------------------------------------------------
// src/sd_manager.c

#include "../include/sd_manager.h"
#include <stdio.h> // Simula el uso de APIs de archivo (como las de FATFS/RTOS)
#include <math.h>

// Placeholder: Simula el descriptor de archivo o manejador de la biblioteca FATFS
static void *g_file_handle = NULL;
static uint32_t g_current_sample_index = 0;

// Inicializa el sistema FATFS y abre el archivo
int SD_MANAGER_init(const char *filename) {
    // Aquí iría:
    // 1. Inicializar la interfaz SD/MMC del HPS.
    // 2. Montar el sistema de archivos FATFS.
    // 3. Abrir el 'filename'.

    // Simulación:
    g_file_handle = (void *)0x1; // Simula que el archivo se abrió correctamente
    g_current_sample_index = 0;

    if (g_file_handle == NULL) {
        return -1; // Fallo al abrir el archivo
    }
    return 0; // Éxito
}

// Lee un bloque de audio del archivo
int SD_MANAGER_read_audio_chunk(audio_buffer_t *buffer) {

    // Aquí iría la llamada a la función de lectura de FATFS:
    // int bytes_read = fatfs_fread(buffer->samples, sizeof(sample_t), BUFFER_SIZE, g_file_handle);

    // Simulación: Genera una onda sinusoidal simple para probar el DSP.
    uint32_t samples_to_read = BUFFER_SIZE;
    uint32_t i;
    int16_t amplitude = 10000;

    for (i = 0; i < samples_to_read; i++) {
        // Genera una sinusoide de 440 Hz (Asumiendo 48kHz de sample rate)
        // El número 109 es para la frecuencia
        buffer->samples[i] = (int16_t)(amplitude * sin(2.0 * 3.14159265 * (g_current_sample_index + i) * 109.0 / 48000.0));
    }

    g_current_sample_index += samples_to_read;
    buffer->num_samples = samples_to_read;

    // Simula que leímos el buffer completo
    return samples_to_read * sizeof(sample_t);
}

// Rebobina el archivo al principio
void SD_MANAGER_rewind() {
    // Aquí iría: fatfs_fseek(g_file_handle, 0, SEEK_SET);
    g_current_sample_index = 0;
}
